#include "PartOne.h"
#include "ActionListener.h"
#include "Exceptions.h"
#include "FormCallbacks.h"
#include "IdManager.h"
#include "JsonUtils.h"
#include "MessageSerializerFactory.h"
#include "MsgType.h"
#include "PacketParser.h"
#include <array>
#include <cassert>
#include <chrono>
#include <type_traits>
#include <vector>

#include "CreateActorMessage.h"
#include "CustomPacketMessage.h"
#include "DestroyActorMessage.h"
#include "HostStopMessage.h"
#include "SetRaceMenuOpenMessage.h"
#include "UpdateGameModeDataMessage.h"

PartOneSendTargetWrapper::PartOneSendTargetWrapper(
  Networking::ISendTarget& underlyingSendTarget_)
  : underlyingSendTarget(underlyingSendTarget_)
{
}

void PartOneSendTargetWrapper::Send(Networking::UserId targetUserId,
                                    Networking::PacketData data, size_t length,
                                    bool reliable)
{
  return underlyingSendTarget.Send(targetUserId, data, length, reliable);
}

void PartOneSendTargetWrapper::Send(Networking::UserId targetUserId,
                                    const IMessageBase& message, bool reliable)
{
  SLNet::BitStream stream;

  PartOne::GetMessageSerializerInstance().Serialize(message, stream);

  Send(targetUserId,
       reinterpret_cast<Networking::PacketData>(stream.GetData()),
       stream.GetNumberOfBytesUsed(), reliable);
}

class FakeSendTarget : public Networking::ISendTarget
{
public:
  void Send(Networking::UserId targetUserId, Networking::PacketData data,
            size_t length, bool reliable) override
  {
    std::shared_ptr<IMessageBase> message;

    auto deserializeResult =
      PartOne::GetMessageSerializerInstance().Deserialize(data, length);
    nlohmann::json j;
    if (deserializeResult) {
      deserializeResult->message->WriteJson(j);
      message = std::move(deserializeResult->message);
    } else {
      std::string s(reinterpret_cast<const char*>(data + 1), length - 1);
      j = nlohmann::json::parse(s);
    }

    messages.push_back(PartOne::Message{ j, message, targetUserId, reliable });
  }

  std::vector<PartOne::Message> messages;
};

struct PartOne::Impl
{
  simdjson::dom::parser parser;
  espm::Loader* espm = nullptr;

  std::function<void(PartOneSendTargetWrapper* sendTarget,
                     MpObjectReference* emitter, MpObjectReference* listener)>
    onSubscribe, onUnsubscribe;

  espm::CompressedFieldsCache compressedFieldsCache;

  std::shared_ptr<PacketParser> packetParser;
  std::shared_ptr<ActionListener> actionListener;

  std::shared_ptr<spdlog::logger> logger;

  std::unique_ptr<PartOneSendTargetWrapper> sendTarget;
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
  Networking::ISendTarget* underlyingSendTargetToSet =
    sendTarget ? sendTarget : &pImpl->fakeSendTarget;

  pImpl->sendTarget.reset(
    new PartOneSendTargetWrapper(*underlyingSendTargetToSet));
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
  TickPacketHistoryPlaybacks();
  TickDeferredMessages();
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

    // Clear actor's hoster if any.
    // HostStop message will be sent on the next attempt to update actor's
    // movement
    // Possible fix for "players link to each other" bug
    // See also ActionListener::SendToNeighbours
    auto hosterActorIt = worldState.hosters.find(actor.GetFormId());
    if (hosterActorIt != worldState.hosters.end()) {
      worldState.hosters.erase(hosterActorIt);
    }

    // Both functions are required here, but it is NOT covered by unit tests
    // properly. If you do something wrong here, players will not be able to
    // interact with items in the same cell after reconnecting.
    actor.UnsubscribeFromAll();
    actor.RemoveFromGridAndUnsubscribeAll();

    serverState.actorsMap.Set(userId, &actor);

    actor.ForceSubscriptionsUpdate();

    // We do the same in MpActor::ApplyChangeForm for non-player characters
    if (actor.IsDead() && !actor.IsRespawning()) {
      spdlog::info("PartOne::SetUserActor {} {:x} - respawning dead actor",
                   userId, actorFormId);
      actor.RespawnWithDelay();
    }

    // This is not currently saved client-side, so reset
    actor.SetLastAnimEvent(std::nullopt);

  } else {
    serverState.actorsMap.Erase(userId);
  }
}

uint32_t PartOne::GetUserActor(Networking::UserId userId)
{
  serverState.EnsureUserExists(userId);

  auto actor = serverState.ActorByUser(userId);
  if (!actor) {
    return 0;
  }
  return actor->GetFormId();
}

std::string PartOne::GetUserGuid(Networking::UserId userId)
{
  serverState.EnsureUserExists(userId);
  return serverState.UserGuid(userId);
}

Networking::UserId PartOne::GetUserByActor(uint32_t formId)
{
  auto& form = worldState.LookupFormById(formId);
  if (form) {
    if (auto ac = form.get()->AsActor()) {
      return serverState.UserByActor(ac);
    }
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

  if (actor.IsRaceMenuOpen() == open) {
    return;
  }

  actor.SetRaceMenuOpen(open);

  auto userId = serverState.UserByActor(&actor);
  if (userId == Networking::InvalidUserId) {
    spdlog::warn(
      "PartOne::SetRaceMenuOpen {:x} - actor is not attached to any of users",
      actorFormId);
    return;
  }

  SetRaceMenuOpenMessage message;
  message.open = open;
  pImpl->sendTarget->Send(userId, message, true);
}

void PartOne::SendCustomPacket(Networking::UserId userId,
                               const std::string& jContent)
{
  CustomPacketMessage message;
  message.contentJsonDump = jContent;
  pImpl->sendTarget->Send(userId, message, true);
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

  auto start = std::chrono::steady_clock::now();

  int n = 0;
  int numPlayerCharacters = 0;
  saveStorage->IterateSync([&](MpChangeForm changeForm) {
    // Do not let players become NPCs
    if (changeForm.profileId != -1 && !changeForm.isDisabled) {
      changeForm.isDisabled = true;
    }

    if (changeForm.isDeleted) {
      pImpl->logger->info(
        "Skipping deleted form {}, will likely overwrite at some point",
        changeForm.formDesc.ToString());
      return;
    }

    bool isFF = changeForm.formDesc.file.empty();

    if (isFF) {
      auto baseId = changeForm.baseDesc.ToFormId(worldState.espmFiles);
      auto lookupRes = GetEspm().GetBrowser().LookupById(baseId);

      if (lookupRes.rec && espm::utils::IsItem(lookupRes.rec->GetType())) {
        pImpl->logger->info("Skipping FF item {} (type is {}), will likely "
                            "overwrite at some point",
                            changeForm.formDesc.ToString(),
                            lookupRes.rec->GetType().ToString());
        return;
      }
      if (lookupRes.rec &&
          (lookupRes.rec->GetType() == "ACTI" ||
           lookupRes.rec->GetType() == "FURN")) {
        pImpl->logger->info("Skipping FF object {} (type is {}), will likely "
                            "overwrite at some point",
                            changeForm.formDesc.ToString(),
                            lookupRes.rec->GetType().ToString());
        return;
      }
    }

    n++;
    worldState.LoadChangeForm(changeForm, CreateFormCallbacks());
    if (changeForm.profileId >= 0) {
      ++numPlayerCharacters;
    }

    if (n % 25 == 0) {
      pImpl->logger->info("Loaded {} ChangeForms", n);
    }
  });

  auto end = std::chrono::steady_clock::now();
  auto duration =
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  pImpl->logger->info("AttachSaveStorage took {} seconds and {} milliseconds, "
                      "loaded {} ChangeForms (Including {} player characters)",
                      duration.count() / 1000, duration.count() % 1000, n,
                      numPlayerCharacters);
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

std::string ExtractGuid(Networking::PacketData data, size_t length)
{
  std::string guid;
  guid.resize(length);
  std::copy(data, data + length, guid.begin());
  return guid;
}

}

void PartOne::HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length)
{
  auto this_ = reinterpret_cast<PartOne*>(partOneInstance);

  switch (packetType) {
    case Networking::PacketType::ServerSideUserConnect: {
      std::string guid = ExtractGuid(data, length);
      return this_->AddUser(userId, UserType::User, guid);
    }
    case Networking::PacketType::ServerSideUserDisconnect: {
      ScopedTask t([userId, this_] {
        if (auto actor = this_->serverState.ActorByUser(userId)) {
          this_->animationSystem.ClearInfo(actor);
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

PartOneSendTargetWrapper& PartOne::GetSendTarget() const
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

float PartOne::CalculateDamage(const MpActor& aggressor, const MpActor& target,
                               const SpellCastData& spellCastData) const
{
  if (!pImpl->damageFormula) {
    throw std::runtime_error("no damage formula");
  }
  return pImpl->damageFormula->CalculateDamage(aggressor, target,
                                               spellCastData);
}

void PartOne::NotifyGamemodeApiStateChanged(
  const GamemodeApi::State& newState) noexcept
{
  UpdateGameModeDataMessage msg;

  for (auto [eventName, eventSourceInfo] : newState.createdEventSources) {
    msg.eventSources.push_back({ eventName, eventSourceInfo.functionBody });
  }

  for (auto [propertyName, propertyInfo] : newState.createdProperties) {
    GamemodeValuePair updateOwnerFunctionsEntry;
    updateOwnerFunctionsEntry.name = propertyName;
    updateOwnerFunctionsEntry.content =
      propertyInfo.isVisibleByOwner ? propertyInfo.updateOwner : "";
    msg.updateOwnerFunctions.push_back(updateOwnerFunctionsEntry);

    //  From docs: isVisibleByNeighbors considered to be always false for
    //  properties with `isVisibleByOwner == false`, in that case, actual
    //  flag value is ignored.

    const bool actuallyVisibleByNeighbor =
      propertyInfo.isVisibleByNeighbors && propertyInfo.isVisibleByOwner;

    GamemodeValuePair updateNeighborFunctionsEntry;
    updateNeighborFunctionsEntry.name = propertyName;
    updateNeighborFunctionsEntry.content =
      actuallyVisibleByNeighbor ? propertyInfo.updateNeighbor : "";
    msg.updateNeighborFunctions.push_back(updateNeighborFunctionsEntry);
  }

  SLNet::BitStream stream;
  GetMessageSerializerInstance().Serialize(msg, stream);

  for (Networking::UserId i = 0; i <= serverState.maxConnectedId; ++i) {
    if (serverState.IsConnected(i)) {
      GetSendTarget().Send(
        i, reinterpret_cast<Networking::PacketData>(stream.GetData()),
        stream.GetNumberOfBytesUsed(), true);
    }
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

void PartOne::SendHostStop(Networking::UserId badHosterUserId,
                           MpObjectReference& remote)
{
  auto remoteAsActor = remote.AsActor();

  uint64_t longFormId = remote.GetFormId();
  if (remoteAsActor && longFormId < 0xff000000) {
    longFormId += 0x100000000;
  }

  HostStopMessage message;
  message.target = longFormId;
  GetSendTarget().Send(badHosterUserId, message, true);
}

FormCallbacks PartOne::CreateFormCallbacks()
{
  auto st = &serverState;

  FormCallbacks::SubscribeCallback
    subscribe =
      [this](MpObjectReference* emitter, MpObjectReference* listener) {
        return pImpl->onSubscribe(pImpl->sendTarget.get(), emitter, listener);
      },
    unsubscribe = [this](MpObjectReference* emitter,
                         MpObjectReference* listener) {
      return pImpl->onUnsubscribe(pImpl->sendTarget.get(), emitter, listener);
    };

  FormCallbacks::SendToUserFn sendToUser =
    [this, st](MpActor* actor, const IMessageBase& message, bool reliable) {
      SLNet::BitStream stream;
      GetMessageSerializerInstance().Serialize(message, stream);

      bool isOffline = st->UserByActor(actor) == Networking::InvalidUserId;

      // Only send to hoster if actor is offline (no active user)
      // This fixes December 2023 Update "invisible chat" bug
      // TODO: make send-to-hoster mechanism explicit, instead of implicitly
      // redirecting packets
      if (isOffline) {
        auto hosterIterator = worldState.hosters.find(actor->GetFormId());
        if (hosterIterator != worldState.hosters.end()) {
          auto& hosterActor =
            worldState.GetFormAt<MpActor>(hosterIterator->second);
          actor = &hosterActor;
          // Send messages such as Teleport, ChangeValues to our host
        }
      }

      auto targetuserId = st->UserByActor(actor);
      if (targetuserId != Networking::InvalidUserId &&
          st->disconnectingUserId != targetuserId) {
        pImpl->sendTarget->Send(
          targetuserId,
          reinterpret_cast<Networking::PacketData>(stream.GetData()),
          stream.GetNumberOfBytesUsed(), reliable);
      }
    };

  FormCallbacks::SendToUserDeferredFn sendToUserDeferred =
    [this, st](MpActor* actor, const IMessageBase& message, bool reliable,
               int deferredChannelId, bool overwritePreviousChannelMessages) {
      SLNet::BitStream stream;
      GetMessageSerializerInstance().Serialize(message, stream);

      if (deferredChannelId < 0 || deferredChannelId >= 100) {
        return spdlog::error(
          "sendToUserDeferred - invalid deferredChannelId {}",
          deferredChannelId);
      }

      auto targetuserId = st->UserByActor(actor);
      if (targetuserId == Networking::InvalidUserId ||
          st->disconnectingUserId == targetuserId) {
        // It's ok, it happens
        return;
      }

      auto& userInfo = st->userInfo[targetuserId];
      if (!userInfo) {
        return spdlog::error("sendToUserDeferred - null userInfo for user {}",
                             targetuserId);
      }

      DeferredMessage deferredMessage;
      deferredMessage.packetData = {
        reinterpret_cast<const Networking::PacketData>(stream.GetData()),
        reinterpret_cast<const Networking::PacketData>(stream.GetData()) +
          stream.GetNumberOfBytesUsed()
      };
      deferredMessage.packetReliable = reliable;
      deferredMessage.actorIdExpected = actor->GetFormId();

      if (userInfo->deferredChannels.size() <= deferredChannelId) {
        userInfo->deferredChannels.resize(deferredChannelId + 1);
      }

      if (overwritePreviousChannelMessages) {
        userInfo->deferredChannels[deferredChannelId] = { deferredMessage };
      } else {
        userInfo->deferredChannels[deferredChannelId].push_back(
          deferredMessage);
      }
    };

  return { subscribe, unsubscribe, sendToUser, sendToUserDeferred };
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

  pImpl->onSubscribe = [this](PartOneSendTargetWrapper* sendTarget,
                              MpObjectReference* emitter,
                              MpObjectReference* listener) {
    if (!emitter) {
      throw std::runtime_error("nullptr emitter in onSubscribe");
    }

    MpActor* listenerAsActor = listener->AsActor();
    if (!listenerAsActor) {
      return;
    }

    auto listenerUserId = serverState.UserByActor(listenerAsActor);
    if (listenerUserId == Networking::InvalidUserId) {
      return;
    }

    auto& emitterPos = emitter->GetPos();
    auto& emitterRot = emitter->GetAngle();

    bool isMe = emitter == listener;

    MpActor* emitterAsActor = emitter->AsActor();

    CreateActorMessage message;

    std::string jAnimation;

    if (emitterAsActor) {
      auto appearance = emitterAsActor->GetAppearance();
      message.appearance = appearance
        ? std::optional<Appearance>(*appearance)
        : std::optional<Appearance>(std::nullopt);
    }

    if (emitterAsActor) {
      message.equipment = emitterAsActor->GetEquipment();
    }

    if (emitterAsActor) {
      message.animation = emitterAsActor->GetLastAnimEvent();
    }

    uint64_t longFormId = emitter->GetFormId();
    if (emitterAsActor && longFormId < 0xff000000) {
      longFormId += 0x100000000;
    }
    message.refrId = longFormId;

    if (emitter->GetBaseId() != 0x00000000 &&
        emitter->GetBaseId() != 0x00000007) {
      message.baseId = emitter->GetBaseId();
    }

    if (emitterAsActor && emitterAsActor->IsDead()) {
      message.isDead = true;
    }

    const bool isOwner = emitter == listener;

    auto mode = VisitPropertiesMode::OnlyPublic;
    if (isOwner) {
      mode = VisitPropertiesMode::All;
    }

    emitter->VisitProperties(message, mode);

    auto isFilteredOut = [&](const CustomPropsEntry& customPropsEntry) {
      auto it = pImpl->gamemodeApiState.createdProperties.find(
        customPropsEntry.propName);
      if (it != pImpl->gamemodeApiState.createdProperties.end()) {
        if (!it->second.isVisibleByOwner) {
          //  From docs: isVisibleByNeighbors is considered to be always false
          //  for properties with `isVisibleByOwner == false`, in that case,
          //  actual flag value is ignored.
          return true;
        }
        if (!it->second.isVisibleByNeighbors && !isOwner) {
          return true;
        }
      }
      return false;
    };

    message.customPropsJsonDumps.erase(
      std::remove_if(message.customPropsJsonDumps.begin(),
                     message.customPropsJsonDumps.end(), isFilteredOut),
      message.customPropsJsonDumps.end());

    const bool hasUser = emitterAsActor &&
      serverState.UserByActor(emitterAsActor) != Networking::InvalidUserId;
    auto hosterIterator = worldState.hosters.find(emitter->GetFormId());

    if (hasUser ||
        (hosterIterator != worldState.hosters.end() &&
         hosterIterator->second != 0 &&
         hosterIterator->second != listener->GetFormId())) {
      message.props.isHostedByOther = true;
    }

    uint32_t worldOrCell =
      emitter->GetCellOrWorld().ToFormId(worldState.espmFiles);

    // See 'perf: improve game framerate #1186'
    // Client needs to know if it is DOOR or not
    if (const std::string& baseType = emitter->GetBaseType();
        baseType == "DOOR") {
      message.baseRecordType = "DOOR";
    }

    message.idx = emitter->GetIdx();
    message.isMe = isMe;
    message.transform.pos = { emitterPos.x, emitterPos.y, emitterPos.z };
    message.transform.rot = { emitterRot.x, emitterRot.y, emitterRot.z };
    message.transform.worldOrCell = worldOrCell;

    sendTarget->Send(listenerUserId, message, true);
  };

  pImpl->onUnsubscribe = [this](PartOneSendTargetWrapper* sendTarget,
                                MpObjectReference* emitter,
                                MpObjectReference* listener) {
    MpActor* listenerAsActor = listener->AsActor();
    if (!listenerAsActor) {
      return;
    }

    auto listenerUserId = serverState.UserByActor(listenerAsActor);
    if (listenerUserId != Networking::InvalidUserId &&
        listenerUserId != serverState.disconnectingUserId) {
      DestroyActorMessage message;
      message.idx = emitter->GetIdx();
      sendTarget->Send(listenerUserId, message, true);
    }
  };
}

void PartOne::AddUser(Networking::UserId userId, UserType type,
                      const std::string& guid)
{
  serverState.Connect(userId, guid);
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
  if (!pImpl->actionListener) {
    pImpl->actionListener = std::make_shared<ActionListener>(*this);
  }
}

void PartOne::TickPacketHistoryPlaybacks()
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
}

void PartOne::TickDeferredMessages()
{
  for (Networking::UserId userId = 0; userId <= serverState.maxConnectedId;
       ++userId) {
    auto& userInfo = serverState.userInfo[userId];
    if (!userInfo) {
      continue;
    }
    for (auto& channel : userInfo->deferredChannels) {
      for (auto& message : channel) {
        auto actor = serverState.ActorByUser(userId);
        if (!actor) {
          continue;
        }

        if (message.actorIdExpected != actor->GetFormId()) {
          continue;
        }

        pImpl->sendTarget->Send(
          userId,
          reinterpret_cast<Networking::PacketData>(message.packetData.data()),
          message.packetData.size(), message.packetReliable);
      }
      channel.clear();
    }
  }
}

MessageSerializer& PartOne::GetMessageSerializerInstance()
{
  static auto g_serializer =
    MessageSerializerFactory::CreateMessageSerializer();
  return *g_serializer;
}
