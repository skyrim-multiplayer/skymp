#include "PartOne.h"
#include "ActionListener.h"
#include "Exceptions.h"
#include "FormCallbacks.h"
#include "IdManager.h"
#include "JsonUtils.h"
#include "MpObjectReference.h"
#include "MsgType.h"
#include "NetworkingInterface.h"
#include "PacketParser.h"
#include <array>
#include <cassert>
#include <chrono>
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
      throw std::runtime_error(fmt::format("{}\n`{}`", e.what(), s));
    }
    messages.push_back(m);
  }

  std::vector<PartOne::Message> messages;
};

struct PartOne::Impl
{
  simdjson::dom::parser parser;
  espm::Loader* espm = nullptr;
  std::function<void(Networking::ISendTarget* sendTarget,
                     uint32_t emitterFormId, uint32_t listenerFormid)>
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

bool PartOne::IsConnected(Networking::UserId userId) const noexcept
{
  return serverState.IsConnected(userId);
}

void PartOne::Tick()
{
  worldState.Tick();
}

uint32_t PartOne::CreateActor(uint32_t formId, const NiPoint3& pos,
                              float angleZ, uint32_t cellOrWorld,
                              ProfileId profileId)
{
  if (!ServerState::Valid(formId)) {
    formId = worldState.GenerateFormId();
  }
  LocationalData locData = { pos,
                             { 0, 0, angleZ },
                             FormDesc::FromFormId(cellOrWorld,
                                                  worldState.espmFiles) };
  auto& actor = worldState.Emplace<MpActor>(formId);
  // ATTENTION
  // QUESTION
  // formId & baseId ??
  auto& objectReference = worldState.Emplace<MpObjectReference>(
    formId, locData, CreateFormCallbacks(),
    !ServerState::Valid(formId) ? 0x7 : formId, "NPC_");
  if (profileId >= 0) {
    objectReference.RegisterProfileId(profileId);
  }
  return formId;
}

void PartOne::SetUserActor(Networking::UserId userId, uint32_t formId)
{
  serverState.EnsureUserExists(userId);

  if (ServerState::Valid(formId)) {
    auto [actor, objectReference] =
      worldState.Get<MpActor, MpObjectReference>(formId);

    if (objectReference.IsDisabled()) {
      throw std::runtime_error(
        fmt::format("Actor with id {:x} is disabled", formId));
    }

    // Both functions are required here, but it is NOT covered by unit tests
    // properly. If you do something wrong here, players would not be able to
    // interact with items in the same cell after reconnecting.
    objectReference.UnsubscribeFromAll();
    objectReference.RemoveFromGrid();
    serverState.Set(userId, formId);
    objectReference.ForceSubscriptionsUpdate();
    if (objectReference.IsDead() && !objectReference.IsRespawning()) {
      actor.RespawnWithDelay();
    }
  } else {
    serverState.Erase(userId);
  }
}

uint32_t PartOne::GetFormIdByUserId(Networking::UserId userId) const
{
  serverState.EnsureUserExists(userId);
  return serverState.GetFormIdByUserId(userId);
}

Networking::UserId PartOne::GetUserIdByFormId(uint32_t formId) const noexcept
{
  if (worldState.Exists(formId)) {
    return serverState.GetUserIdByFormId(formId);
  };
  return Networking::InvalidUserId;
}

void PartOne::DestroyActor(uint32_t actorFormId)
{
  worldState.Destroy(actorFormId);
  serverState.Erase(actorFormId);
}

void PartOne::SetRaceMenuOpen(uint32_t formId, bool open)
{
  auto [actor, objectReference] =
    worldState.Get<MpActor, MpObjectReference>(formId);
  if (objectReference.IsRaceMenuOpen() == open) {
    return;
  }
  objectReference.SetRaceMenuOpen(open);
  Networking::UserId userId =
    serverState.GetUserIdByFormId(objectReference.GetFormId());
  if (!ServerState::Valid(userId)) {
    throw std::runtime_error(fmt::format(
      "Actor with id {:#x} is not attached to any of users", formId));
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

std::string PartOne::GetActorName(uint32_t formId)
{
  auto& ac = worldState.Get<MpActor>(formId);
  return ac.GetAppearance() ? ac.GetAppearance()->name : "Prisoner";
}

NiPoint3 PartOne::GetActorPos(uint32_t formId)
{
  auto [ac, objectReference] =
    worldState.Get<MpActor, MpObjectReference>(formId);
  return objectReference.GetPos();
}

uint32_t PartOne::GetActorCellOrWorld(uint32_t formId)
{
  auto [ac, objectReference] =
    worldState.Get<MpActor, MpObjectReference>(formId);
  return objectReference.GetCellOrWorld().ToFormId(worldState.espmFiles);
}

const std::set<uint32_t>& PartOne::GetActorsByProfileId(ProfileId profileId)
{
  return worldState.GetActorsByProfileId(profileId);
}

void PartOne::SetEnabled(uint32_t formId, bool enabled)
{
  auto [ac, objectReference] =
    worldState.Get<MpActor, MpObjectReference>(formId);
  enabled ? objectReference.Enable() : objectReference.Disable();
}

void PartOne::AttachEspm(espm::Loader* espm)
{
  pImpl->espm = espm;
  worldState.AttachEspm(espm, [this] { return CreateFormCallbacks(); });
}

void PartOne::AttachSaveStorage(std::shared_ptr<ISaveStorage> saveStorage)
{
  worldState.AttachSaveStorage(saveStorage);

  auto was = std::chrono::steady_clock::now();

  int n = 0;
  int numPlayerCharacters = 0;
  saveStorage->IterateSync([&](MpChangeForm changeForm) {
    // Do not let players become NPCs
    if (changeForm.profileId != -1 && !changeForm.isDisabled) {
      changeForm.isDisabled = true;
    }

    n++;
    worldState.LoadChangeForm(changeForm, CreateFormCallbacks());
    if (changeForm.profileId >= 0) {
      ++numPlayerCharacters;
    }
  });

  auto duration = std::chrono::steady_clock::now() - was;
  pImpl->logger->info("AttachSaveStorage took {} ticks, loaded {} ChangeForms "
                      "(Including {} player characters)",
                      duration.count(), n, numPlayerCharacters);
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
  auto _this = reinterpret_cast<PartOne*>(partOneInstance);

  switch (packetType) {
    case Networking::PacketType::ServerSideUserConnect:
      return _this->AddUser(userId, UserType::User);
    case Networking::PacketType::ServerSideUserDisconnect: {
      ScopedTask t([userId, _this] {
        // formId or entitt ??
        const uint32_t formId = _this->serverState.GetFormIdByUserId(userId);
        auto [actor, objectReference] =
          _this->worldState.Get<MpActor, MpObjectReference>(formId);
        if (_this->animationSystem) {
          _this->animationSystem->ClearInfo(objectReference);
        }
        _this->serverState.Disconnect(userId);
        _this->serverState.disconnectingUserId = Networking::InvalidUserId;
      });

      _this->serverState.disconnectingUserId = userId;
      for (const auto& listener : _this->worldState.GetListeners()) {
        listener->OnDisconnect(userId);
      }
      return;
    }
    case Networking::PacketType::Message:
      return _this->HandleMessagePacket(userId, data, length);
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
    if (!serverState.IsConnected(i)) {
      continue;
    }
    GetSendTarget().Send(i, reinterpret_cast<Networking::PacketData>(m.data()),
                         m.size(), true);
  }

  pImpl->gamemodeApiState = newState;
}

FormCallbacks PartOne::CreateFormCallbacks()
{
  FormCallbacks::SubscribeCallback
    subscribe =
      [this](uint32_t emitterFormId, uint32_t listenerFormId) {
        return pImpl->onSubscribe(pImpl->sendTarget, emitterFormId,
                                  listenerFormId);
      },
    unsubscribe = [this](uint32_t emitterFormId, uint32_t listenerFormId) {
      return pImpl->onUnsubscribe(pImpl->sendTarget, emitterFormId,
                                  listenerFormId);
    };

  FormCallbacks::SendToUserFn sendToUser = [this, pServerState = &serverState](
                                             uint32_t targetFormId,
                                             const void* data, size_t size,
                                             bool reliable) {
    Networking::UserId targetUserId =
      pServerState->GetUserIdByFormId(targetFormId);
    if (ServerState::Valid(targetUserId) &&
        !pServerState->IsUserDisconnecting(targetUserId)) {
      const auto& packetData = reinterpret_cast<Networking::PacketData>(data);
      pImpl->sendTarget->Send(targetUserId, packetData, size, reliable);
    }
  };
  return { subscribe, unsubscribe, sendToUser };
}

ActionListener& PartOne::GetActionListener()
{
  InitActionListener();
  return *pImpl->actionListener;
}

const std::vector<std::shared_ptr<PartOne::Listener>>& PartOne::GetListeners()
  const noexcept
{
  return worldState.GetListeners();
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
                              uint32_t emitterFormId,
                              uint32_t listenerFormId) {
    auto [emitterActor, emitterObjectReference] =
      worldState.Get<MpActor, MpObjectReference>(emitterFormId);
    auto [listenerActor, listenerObjectReference] =
      worldState.Get<MpActor, MpObjectReference>(listenerFormId);

    Networking::UserId listenerUserId =
      serverState.GetUserIdByFormId(listenerObjectReference.GetFormId());
    if (!ServerState::Valid(listenerUserId)) {
      return;
    }
    const NiPoint3& emitterPos = emitterObjectReference.GetPos();
    const NiPoint3& emitterRot = emitterObjectReference.GetAngle();

    bool isMe = emitterObjectReference.GetFormId() ==
      listenerObjectReference.GetFormId();

    std::string jEquipment, jAppearance;
    const char *appearancePrefix = "", *appearance = "";
    jAppearance = emitterActor.GetAppearanceAsJson();
    if (!jAppearance.empty()) {
      appearancePrefix = R"(, "appearance": )";
      appearance = jAppearance.data();
    }

    const char *equipmentPrefix = "", *equipment = "";
    jEquipment = emitterActor.GetEquipmentAsJson();
    if (!jEquipment.empty()) {
      equipmentPrefix = R"(, "equipment": )";
      equipment = jEquipment.data();
    }

    const char* refrIdPrefix = "";
    char refrId[32] = { 0 };
    refrIdPrefix = R"(, "refrId": )";

    unsigned long long longFormId = emitterObjectReference.GetFormId();
    if (longFormId < 0xff000000) {
      longFormId += 0x100000000;
    }
    sprintf(refrId, "%llu", longFormId);

    const char* baseIdPrefix = "";
    char baseId[32] = { 0 };
    if (emitterObjectReference.GetBaseId() != 0x00000000 &&
        emitterObjectReference.GetBaseId() != 0x00000007) {
      baseIdPrefix = R"(, "baseId": )";
      sprintf(baseId, "%u", emitterObjectReference.GetBaseId());
    }

    const bool isOwner = emitterObjectReference.GetBaseId() ==
      listenerObjectReference.GetBaseId();

    std::string props;
    auto mode = VisitPropertiesMode::OnlyPublic;
    if (isOwner) {
      mode = VisitPropertiesMode::All;
    }

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

      if (props.size() > 0) {
        props += R"(, ")";
      } else {
        props += '"';
      }
      props += propName;
      props += R"(": )";
      props += jsonValue;
    };
    emitterObjectReference.VisitProperties(visitor, mode);

    const bool hasUser = ServerState::Valid(
      serverState.GetUserIdByFormId(emitterObjectReference.GetFormId()));

    auto hosterIterator =
      worldState.hosters.find(emitterObjectReference.GetFormId());
    if (hasUser ||
        (hosterIterator != worldState.hosters.end() &&
         hosterIterator->second != 0 &&
         hosterIterator->second != listenerObjectReference.GetFormId())) {
      visitor("isHostedByOther", "true");
    }

    const char* method = "createActor";

    uint32_t worldOrCell =
      emitterObjectReference.GetCellOrWorld().ToFormId(worldState.espmFiles);

    // See 'perf: improve game framerate #1186'
    // Client needs to know if it is DOOR or not
    const char* baseRecordTypePrefix = "";
    std::string baseRecordType;
    if (const std::string& baseType = emitterObjectReference.GetBaseType();
        baseType == "DOOR") {
      baseRecordTypePrefix = R"(, "baseRecordType": )";
      baseRecordType = '"' + baseType + '"';
    }

    Networking::SendFormatted(
      sendTarget, listenerUserId,
      R"({"type": "%s", "idx": %u, "isMe": %s, "transform": {"pos":
    [%f,%f,%f], "rot": [%f,%f,%f], "worldOrCell": %u}%s%s%s%s%s%s%s%s%s%s%s%s%s})",
      method, emitterObjectReference.GetIdx(), isMe ? "true" : "false",
      emitterPos.x, emitterPos.y, emitterPos.z, emitterRot.x, emitterRot.y,
      emitterRot.z, worldOrCell, baseRecordTypePrefix, baseRecordType.data(),
      appearancePrefix, appearance, equipmentPrefix, equipment, refrIdPrefix,
      refrId, baseIdPrefix, baseId, propsPrefix, props.data(), propsPostfix);
  };

  pImpl->onUnsubscribe = [this](Networking::ISendTarget* sendTarget,
                                uint32_t emitterFormId,
                                uint32_t listenerFormId) {
    auto [listenerActor, listenerObjectReference] =
      worldState.Get<MpActor, MpObjectReference>(listenerFormId);
    auto [emitterActor, emitterObjectReference] =
      worldState.Get<MpActor, MpObjectReference>(listenerFormId);
    const Networking::UserId listenerUserId =
      serverState.GetUserIdByFormId(listenerObjectReference.GetFormId());
    if (ServerState::Valid(listenerUserId) &&
        listenerUserId != serverState.disconnectingUserId) {
      Networking::SendFormatted(sendTarget, listenerUserId,
                                R"({"type": "destroyActor", "idx": %u})",
                                emitterObjectReference.GetIdx());
    }
  };
}

void PartOne::AddUser(Networking::UserId userId, UserType type)
{
  serverState.Connect(userId);
  for (const auto& listener : worldState.GetListeners()) {
    listener->OnConnect(userId);
  }

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
    throw std::runtime_error(
      fmt::format("User with id {:x} does not exist", userId));
  }
  if (!pImpl->packetParser) {
    pImpl->packetParser.reset(new PacketParser);
  }
  InitActionListener();
  pImpl->packetParser->TransformPacketIntoAction(userId, data, length,
                                                 *pImpl->actionListener);
}

void PartOne::InitActionListener()
{
  if (!pImpl->actionListener) {
    pImpl->actionListener.reset(new ActionListener(*this));
  }
}
