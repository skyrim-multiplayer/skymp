#include "PartOne.h"
#include "ActionListener.h"
#include "Exceptions.h"
#include "FormCallbacks.h"
#include "IdManager.h"
#include "JsonUtils.h"
#include "MsgType.h"
#include "PacketParser.h"
#include <array>
#include <cassert>
#include <type_traits>
#include <vector>

class FakeSendTarget : public Networking::ISendTarget
{
public:
  void Send(Networking::UserId targetUserId, Networking::PacketData data,
            size_t length, bool reliable) override
  {
    std::string s(reinterpret_cast<const char*>(data + 1), length - 1);
    PartOne::Message m;
    try {
      m = PartOne::Message{ nlohmann::json::parse(s), targetUserId, reliable };
    } catch (std::exception& e) {
      std::stringstream ss;
      ss << e.what() << std::endl << "`" << s << "`";
      throw std::runtime_error(ss.str());
    }
    messages.push_back(m);
  }

  std::vector<PartOne::Message> messages;
};

struct PartOne::Impl
{
  simdjson::dom::parser parser;
  espm::Loader* espm = nullptr;

  std::function<void(Networking::ISendTarget*sendTarget,
                     MpObjectReference*emitter, MpObjectReference*listener)>
    onSubscribe, onUnsubscribe;

  espm::CompressedFieldsCache compressedFieldsCache;

  std::shared_ptr<PacketParser> packetParser;
  std::shared_ptr<ActionListener> actionListener;

  std::shared_ptr<spdlog::logger> logger;

  Networking::ISendTarget* sendTarget = nullptr;
  std::unique_ptr<IDamageFormula> damageFormula{};
  FakeSendTarget fakeSendTarget;

  GamemodeApi::State gamemodeApiState;
  std::string updateGamemodeDataMsg;
};

PartOne::PartOne(Networking::ISendTarget* sendTarget)
{
  Init();
  SetSendTarget(sendTarget);
}

PartOne::PartOne(std::shared_ptr<Listener> listener,
                 Networking::ISendTarget* sendTarget)
{
  Init();
  AddListener(listener);
  SetSendTarget(sendTarget);
}

PartOne::~PartOne()
{
  // worldState may depend on serverState (actorsMap), we should reset it first
  worldState.Clear();
  serverState = {};
}

void PartOne::SetSendTarget(Networking::ISendTarget* sendTarget)
{
  pImpl->sendTarget = sendTarget ? sendTarget : &pImpl->fakeSendTarget;
}

void PartOne::SetDamageFormula(std::unique_ptr<IDamageFormula> dmgFormula)
{
  pImpl->damageFormula = std::move(dmgFormula);
}

void PartOne::AddListener(std::shared_ptr<Listener> listener)
{
  worldState.listeners.push_back(listener);
}

bool PartOne::IsConnected(Networking::UserId userId) const
{
  return serverState.IsConnected(userId);
}

void PartOne::Tick()
{
  for (auto& [userId, playback] : serverState.requestedPlaybacks) {
    serverState.activePlaybacks[userId] = std::move(playback);
  }
  serverState.requestedPlaybacks.clear();

  for (auto& [userId, playback] : serverState.activePlaybacks) {
    auto& packetHistory = playback.history;

    while (
      !packetHistory.packets.empty() &&
      playback.startTime +
          std::chrono::milliseconds(packetHistory.packets.front().timeMs) <=
        std::chrono::steady_clock::now()) {
      auto& packet = packetHistory.packets.front();

      if (packetHistory.buffer.size() < packet.offset + packet.length) {
        spdlog::error("Packet history buffer is corrupted");
      } else {
        pImpl->packetParser->TransformPacketIntoAction(
          userId, &packetHistory.buffer[packet.offset], packet.length,
          *pImpl->actionListener);
      }
      packetHistory.packets.pop_front();
    }
  }

  // delete playback if packetHistory.packets is empty
  for (auto it = serverState.activePlaybacks.begin();
       it != serverState.activePlaybacks.end();) {
    if (it->second.history.packets.empty()) {
      it = serverState.activePlaybacks.erase(it);
    } else {
      ++it;
    }
  }
  worldState.Tick();
}

uint32_t PartOne::CreateActor(uint32_t formId, const NiPoint3& pos,
                              float angleZ, uint32_t cellOrWorld,
                              ProfileId profileId)
{
  if (!formId) {
    formId = worldState.GenerateFormId();
  }
  worldState.AddForm(
    std::unique_ptr<MpActor>(
      new MpActor({ pos,
                    { 0, 0, angleZ },
                    FormDesc::FromFormId(cellOrWorld, worldState.espmFiles) },
                  CreateFormCallbacks())),
    formId);
  if (profileId >= 0) {
    auto& ac = worldState.GetFormAt<MpActor>(formId);
    ac.RegisterProfileId(profileId);
  }

  return formId;
}

void PartOne::SetUserActor(Networking::UserId userId, uint32_t actorFormId)
{
  serverState.EnsureUserExists(userId);

  if (actorFormId > 0) {
    auto& actor = worldState.GetFormAt<MpActor>(actorFormId);

    if (actor.IsDisabled()) {
      std::stringstream ss;
      ss << "Actor with id " << std::hex << actorFormId << " is disabled";
      throw std::runtime_error(ss.str());
    }

    // Both functions are required here, but it is NOT covered by unit tests
    // properly. If you do something wrong here, players would not be able to
    // interact with items in the same cell after reconnecting.
    actor.UnsubscribeFromAll();
    actor.RemoveFromGrid();

    serverState.actorsMap.Set(userId, &actor);

    actor.ForceSubscriptionsUpdate();

    if (actor.IsDead() && !actor.IsRespawning()) {
      actor.RespawnWithDelay();
    }

  } else {
    serverState.actorsMap.Erase(userId);
  }
}

uint32_t PartOne::GetUserActor(Networking::UserId userId)
{
  serverState.EnsureUserExists(userId);

  auto actor = serverState.ActorByUser(userId);
  if (!actor)
    return 0;
  return actor->GetFormId();
}

Networking::UserId PartOne::GetUserByActor(uint32_t formId)
{
  auto& form = worldState.LookupFormById(formId);
  if (auto ac = dynamic_cast<MpActor*>(form.get())) {
    return serverState.UserByActor(ac);
  }
  return Networking::InvalidUserId;
}

void PartOne::DestroyActor(uint32_t actorFormId)
{
  std::shared_ptr<MpActor> destroyedForm;
  worldState.DestroyForm<MpActor>(actorFormId, &destroyedForm);

  serverState.actorsMap.Erase(destroyedForm.get());
}

void PartOne::SetRaceMenuOpen(uint32_t actorFormId, bool open)
{
  auto& actor = worldState.GetFormAt<MpActor>(actorFormId);

  if (actor.IsRaceMenuOpen() == open)
    return;

  actor.SetRaceMenuOpen(open);

  auto userId = serverState.UserByActor(&actor);
  if (userId == Networking::InvalidUserId) {
    throw std::runtime_error(fmt::format(
      "Actor with id {:#x} is not attached to any of users", actorFormId));
  }

  Networking::SendFormatted(pImpl->sendTarget, userId,
                            R"({"type": "setRaceMenuOpen", "open": %s})",
                            open ? "true" : "false");
}

void PartOne::SendCustomPacket(Networking::UserId userId,
                               const std::string& jContent)
{
  Networking::SendFormatted(pImpl->sendTarget, userId,
                            R"({"type": "customPacket", "content":%s})",
                            jContent.data());
}

std::string PartOne::GetActorName(uint32_t actorFormId)
{
  auto& ac = worldState.GetFormAt<MpActor>(actorFormId);
  return ac.GetAppearance() ? ac.GetAppearance()->name : "Prisoner";
}

NiPoint3 PartOne::GetActorPos(uint32_t actorFormId)
{
  auto& ac = worldState.GetFormAt<MpActor>(actorFormId);
  return ac.GetPos();
}

uint32_t PartOne::GetActorCellOrWorld(uint32_t actorFormId)
{
  auto& ac = worldState.GetFormAt<MpActor>(actorFormId);
  return ac.GetCellOrWorld().ToFormId(worldState.espmFiles);
}

const std::set<uint32_t>& PartOne::GetActorsByProfileId(ProfileId profileId)
{
  return worldState.GetActorsByProfileId(profileId);
}

void PartOne::SetEnabled(uint32_t actorFormId, bool enabled)
{
  auto& ac = worldState.GetFormAt<MpActor>(actorFormId);
  enabled ? ac.Enable() : ac.Disable();
}

void PartOne::AttachEspm(espm::Loader* espm)
{
  pImpl->espm = espm;
  worldState.AttachEspm(espm, [this] { return CreateFormCallbacks(); });
}

void PartOne::AttachSaveStorage(std::shared_ptr<ISaveStorage> saveStorage)
{
  worldState.AttachSaveStorage(saveStorage);

  clock_t was = clock();

  int n = 0;
  int numPlayerCharacters = 0;
  saveStorage->IterateSync([&](MpChangeForm changeForm) {
    // Do not let players become NPCs
    if (changeForm.profileId != -1 && !changeForm.isDisabled) {
      changeForm.isDisabled = true;
    }

    n++;
    worldState.LoadChangeForm(changeForm, CreateFormCallbacks());
    if (changeForm.profileId >= 0)
      ++numPlayerCharacters;
  });

  pImpl->logger->info("AttachSaveStorage took {} ticks, loaded {} ChangeForms "
                      "(Including {} player characters)",
                      clock() - was, n, numPlayerCharacters);
}

espm::Loader& PartOne::GetEspm() const
{
  return worldState.GetEspm();
}

bool PartOne::HasEspm() const
{
  return !worldState.espmFiles.empty();
}

void PartOne::AttachLogger(std::shared_ptr<spdlog::logger> logger)
{
  pImpl->logger = logger;
  worldState.logger = logger;
}

spdlog::logger& PartOne::GetLogger()
{
  if (!pImpl->logger) {
    throw std::runtime_error("no logger attached");
  }
  return *pImpl->logger;
}

namespace {
class ScopedTask
{
public:
  ScopedTask(std::function<void()> f_)
    : f(f_)
  {
  }
  ~ScopedTask() { f(); }

private:
  const std::function<void()> f;
};
}

void PartOne::HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length)
{
  auto this_ = reinterpret_cast<PartOne*>(partOneInstance);

  switch (packetType) {
    case Networking::PacketType::ServerSideUserConnect:
      return this_->AddUser(userId, UserType::User);
    case Networking::PacketType::ServerSideUserDisconnect: {
      ScopedTask t([userId, this_] {
        if (auto actor = this_->serverState.ActorByUser(userId)) {
          if (this_->animationSystem) {
            this_->animationSystem->ClearInfo(actor);
          }
        }
        this_->serverState.Disconnect(userId);
        this_->serverState.disconnectingUserId = Networking::InvalidUserId;
      });

      this_->serverState.disconnectingUserId = userId;
      for (auto& listener : this_->worldState.listeners)
        listener->OnDisconnect(userId);
      return;
    }
    case Networking::PacketType::Message:
      return this_->HandleMessagePacket(userId, data, length);
    default:
      throw std::runtime_error("Unexpected PacketType: " +
                               std::to_string((int)packetType));
  }
}

Networking::ISendTarget& PartOne::GetSendTarget() const
{
  if (!pImpl->sendTarget) {
    throw std::runtime_error("No send target found");
  }
  return *pImpl->sendTarget;
}

float PartOne::CalculateDamage(const MpActor& aggressor, const MpActor& target,
                               const HitData& hitData) const
{
  if (!pImpl->damageFormula) {
    throw std::runtime_error("no damage formula");
  }
  return pImpl->damageFormula->CalculateDamage(aggressor, target, hitData);
}

void PartOne::NotifyGamemodeApiStateChanged(
  const GamemodeApi::State& newState) noexcept
{
  nlohmann::json j{ { "type", "updateGamemodeData" },
                    { "eventSources", nlohmann::json::object() },
                    { "updateOwnerFunctions", nlohmann::json::object() } };
  for (auto [eventName, eventSourceInfo] : newState.createdEventSources) {
    j["eventSources"][eventName] = eventSourceInfo.functionBody;
  }
  for (auto [propertyName, propertyInfo] : newState.createdProperties) {
    //  From docs: isVisibleByNeighbors considered to be always false for
    //  properties with `isVisibleByOwner == false`, in that case, actual
    //  flag value is ignored.
    const bool actuallyVisibleByNeighbor =
      propertyInfo.isVisibleByNeighbors && propertyInfo.isVisibleByOwner;

    j["updateOwnerFunctions"][propertyName] =
      propertyInfo.isVisibleByOwner ? propertyInfo.updateOwner : "";
    j["updateNeighborFunctions"][propertyName] =
      actuallyVisibleByNeighbor ? propertyInfo.updateNeighbor : "";
  }

  std::string m;
  m += Networking::MinPacketId;
  m += j.dump();
  pImpl->updateGamemodeDataMsg = m;

  for (Networking::UserId i = 0; i <= serverState.maxConnectedId; ++i) {
    if (!serverState.IsConnected(i))
      continue;
    GetSendTarget().Send(i, reinterpret_cast<Networking::PacketData>(m.data()),
                         m.size(), true);
  }

  pImpl->gamemodeApiState = newState;
}

void PartOne::SetPacketHistoryRecording(Networking::UserId userId, bool enable)
{
  if (userId < serverState.userInfo.size() && serverState.userInfo[userId]) {
    if (!serverState.userInfo[userId]->packetHistoryStartTime) {
      serverState.userInfo[userId]->packetHistoryStartTime =
        std::chrono::steady_clock::now();
    }
    serverState.userInfo[userId]->isPacketHistoryRecording = enable;
  } else {
    throw std::runtime_error("Invalid user id " + std::to_string(userId));
  }
}

PacketHistory PartOne::GetPacketHistory(Networking::UserId userId)
{
  if (userId < serverState.userInfo.size() && serverState.userInfo[userId]) {
    return serverState.userInfo[userId]->packetHistory;
  } else {
    throw std::runtime_error("Invalid user id " + std::to_string(userId));
  }
}

void PartOne::ClearPacketHistory(Networking::UserId userId)
{
  if (userId < serverState.userInfo.size() && serverState.userInfo[userId]) {
    serverState.userInfo[userId]->packetHistory = std::move(PacketHistory{});
    serverState.userInfo[userId]->packetHistoryStartTime = std::nullopt;
  } else {
    throw std::runtime_error("Invalid user id " + std::to_string(userId));
  }
}

void PartOne::RequestPacketHistoryPlayback(Networking::UserId userId,
                                           const PacketHistory& history)
{
  if (userId < serverState.userInfo.size() && serverState.userInfo[userId]) {
    serverState.requestedPlaybacks[userId] =
      Playback{ history, std::chrono::steady_clock::now() };
  } else {
    throw std::runtime_error("Invalid user id " + std::to_string(userId));
  }
}

FormCallbacks PartOne::CreateFormCallbacks()
{
  auto st = &serverState;

  FormCallbacks::SubscribeCallback
    subscribe =
      [this](MpObjectReference*emitter, MpObjectReference*listener) {
        return pImpl->onSubscribe(pImpl->sendTarget, emitter, listener);
      },
    unsubscribe = [this](MpObjectReference*emitter,
                         MpObjectReference*listener) {
      return pImpl->onUnsubscribe(pImpl->sendTarget, emitter, listener);
    };

  FormCallbacks::SendToUserFn sendToUser =
    [this, st](MpActor* actor, const void* data, size_t size, bool reliable) {
      auto targetuserId = st->UserByActor(actor);
      if (targetuserId != Networking::InvalidUserId &&
          st->disconnectingUserId != targetuserId)
        pImpl->sendTarget->Send(targetuserId,
                                reinterpret_cast<Networking::PacketData>(data),
                                size, reliable);
    };

  return { subscribe, unsubscribe, sendToUser };
}

ActionListener& PartOne::GetActionListener()
{
  InitActionListener();
  return *pImpl->actionListener;
}

const std::vector<std::shared_ptr<PartOne::Listener>>& PartOne::GetListeners()
  const
{
  return worldState.listeners;
}

std::vector<PartOne::Message>& PartOne::Messages()
{
  return pImpl->fakeSendTarget.messages;
}

void PartOne::Init()
{
  pImpl.reset(new Impl);
  pImpl->logger.reset(new spdlog::logger{ "empty logger" });

  pImpl->onSubscribe = [this](Networking::ISendTarget* sendTarget,
                              MpObjectReference* emitter,
                              MpObjectReference* listener) {
    if (!emitter)
      throw std::runtime_error("nullptr emitter in onSubscribe");
    auto listenerAsActor = dynamic_cast<MpActor*>(listener);
    if (!listenerAsActor)
      return;
    auto listenerUserId = serverState.UserByActor(listenerAsActor);
    if (listenerUserId == Networking::InvalidUserId)
      return;

    auto& emitterPos = emitter->GetPos();
    auto& emitterRot = emitter->GetAngle();

    bool isMe = emitter == listener;

    auto emitterAsActor = dynamic_cast<MpActor*>(emitter);

    std::string jEquipment, jAppearance;

    const char *appearancePrefix = "", *appearance = "";
    if (emitterAsActor) {
      jAppearance = emitterAsActor->GetAppearanceAsJson();
      if (!jAppearance.empty()) {
        appearancePrefix = R"(, "appearance": )";
        appearance = jAppearance.data();
      }
    }

    const char *equipmentPrefix = "", *equipment = "";
    if (emitterAsActor) {
      jEquipment = emitterAsActor->GetEquipmentAsJson();
      if (!jEquipment.empty()) {
        equipmentPrefix = R"(, "equipment": )";
        equipment = jEquipment.data();
      }
    }

    const char* refrIdPrefix = "";
    char refrId[32] = { 0 };
    refrIdPrefix = R"(, "refrId": )";

    long long unsigned int longFormId = emitter->GetFormId();
    if (emitterAsActor && longFormId < 0xff000000) {
      longFormId += 0x100000000;
    }
    sprintf(refrId, "%llu", longFormId);

    const char* baseIdPrefix = "";
    char baseId[32] = { 0 };
    if (emitter->GetBaseId() != 0x00000000 &&
        emitter->GetBaseId() != 0x00000007) {
      baseIdPrefix = R"(, "baseId": )";
      sprintf(baseId, "%u", emitter->GetBaseId());
    }

    const bool isOwner = emitter == listener;

    std::string props;

    auto mode = VisitPropertiesMode::OnlyPublic;
    if (isOwner)
      mode = VisitPropertiesMode::All;

    const char *propsPrefix = "", *propsPostfix = "";
    auto visitor = [&](const char* propName, const char* jsonValue) {
      auto it = pImpl->gamemodeApiState.createdProperties.find(propName);
      if (it != pImpl->gamemodeApiState.createdProperties.end()) {
        if (!it->second.isVisibleByOwner) {
          //  From docs: isVisibleByNeighbors considered to be always false for
          //  properties with `isVisibleByOwner == false`, in that case, actual
          //  flag value is ignored.
          return;
        }

        if (!it->second.isVisibleByNeighbors && !isOwner) {
          return;
        }
      }

      propsPrefix = R"(, "props": { )";
      propsPostfix = R"( })";

      if (props.size() > 0)
        props += R"(, ")";
      else
        props += '"';
      props += propName;
      props += R"(": )";
      props += jsonValue;
    };
    emitter->VisitProperties(visitor, mode);

    const bool hasUser = emitterAsActor &&
      serverState.UserByActor(emitterAsActor) != Networking::InvalidUserId;
    auto hosterIterator = worldState.hosters.find(emitter->GetFormId());

    if (hasUser ||
        (hosterIterator != worldState.hosters.end() &&
         hosterIterator->second != 0 &&
         hosterIterator->second != listener->GetFormId())) {
      visitor("isHostedByOther", "true");
    }

    const char* method = "createActor";

    uint32_t worldOrCell =
      emitter->GetCellOrWorld().ToFormId(worldState.espmFiles);

    // See 'perf: improve game framerate #1186'
    // Client needs to know if it is DOOR or not
    const char* baseRecordTypePrefix = "";
    std::string baseRecordType;
    if (const std::string& baseType = emitter->GetBaseType();
        baseType == "DOOR") {
      baseRecordTypePrefix = R"(, "baseRecordType": )";
      baseRecordType = '"' + baseType + '"';
    }

    Networking::SendFormatted(
      sendTarget, listenerUserId,
      R"({"type": "%s", "idx": %u, "isMe": %s, "transform": {"pos":
    [%f,%f,%f], "rot": [%f,%f,%f], "worldOrCell": %u}%s%s%s%s%s%s%s%s%s%s%s%s%s})",
      method, emitter->GetIdx(), isMe ? "true" : "false", emitterPos.x,
      emitterPos.y, emitterPos.z, emitterRot.x, emitterRot.y, emitterRot.z,
      worldOrCell, baseRecordTypePrefix, baseRecordType.data(),
      appearancePrefix, appearance, equipmentPrefix, equipment, refrIdPrefix,
      refrId, baseIdPrefix, baseId, propsPrefix, props.data(), propsPostfix);
  };

  pImpl->onUnsubscribe = [this](Networking::ISendTarget* sendTarget,
                                MpObjectReference* emitter,
                                MpObjectReference* listener) {
    auto listenerAsActor = dynamic_cast<MpActor*>(listener);
    if (!listenerAsActor)
      return;

    auto listenerUserId = serverState.UserByActor(listenerAsActor);
    if (listenerUserId != Networking::InvalidUserId &&
        listenerUserId != serverState.disconnectingUserId)
      Networking::SendFormatted(sendTarget, listenerUserId,
                                R"({"type": "destroyActor", "idx": %u})",
                                emitter->GetIdx());
  };
}

void PartOne::AddUser(Networking::UserId userId, UserType type)
{
  serverState.Connect(userId);
  for (auto& listener : worldState.listeners)
    listener->OnConnect(userId);

  if (!pImpl->updateGamemodeDataMsg.empty()) {
    GetSendTarget().Send(userId,
                         reinterpret_cast<Networking::PacketData>(
                           pImpl->updateGamemodeDataMsg.data()),
                         pImpl->updateGamemodeDataMsg.size(), true);
  }
}

void PartOne::HandleMessagePacket(Networking::UserId userId,
                                  Networking::PacketData data, size_t length)
{
  if (!serverState.IsConnected(userId)) {
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");
  }
  if (!pImpl->packetParser) {
    pImpl->packetParser = std::make_shared<PacketParser>();
  }

  InitActionListener();

  auto& userInfo = serverState.userInfo[userId];
  if (userInfo && userInfo->isPacketHistoryRecording) {
    if (!userInfo->packetHistoryStartTime) {
      spdlog::error(
        "Expected packetHistoryStartTime to present, probably incorrect code");
    } else {
      size_t offset = userInfo->packetHistory.buffer.size();

      userInfo->packetHistory.buffer.resize(offset + length);
      std::copy(data, data + length,
                userInfo->packetHistory.buffer.data() + offset);

      auto duration =
        std::chrono::steady_clock::now() - *userInfo->packetHistoryStartTime;
      auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration);
      auto timeMs = milliseconds.count();

      userInfo->packetHistory.packets.push_back(
        { offset, length, static_cast<uint64_t>(timeMs) });
    }
  }

  if (serverState.activePlaybacks.count(userId) > 0) {
    return;
  }

  pImpl->packetParser->TransformPacketIntoAction(userId, data, length,
                                                 *pImpl->actionListener);
}

void PartOne::InitActionListener()
{
  if (!pImpl->actionListener)
    pImpl->actionListener.reset(new ActionListener(*this));
}
