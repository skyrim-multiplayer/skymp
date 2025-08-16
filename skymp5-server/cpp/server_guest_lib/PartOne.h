#pragma once
#include "AnimationSystem.h"
#include "GamemodeApi.h"
#include "HitData.h"
#include "MessageEvent.h"
#include "MpActor.h"
#include "Networking.h"
#include "NiPoint3.h"
#include "PartOneListener.h"
#include "RawMessageData.h"
#include "ServerState.h"
#include "SpellCastData.h"
#include "WorldState.h"
#include "formulas/IDamageFormula.h"
#include "libespm/Loader.h"
#include "save_storages/ISaveStorage.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <simdjson.h>
#include <spdlog/logger.h>
#include <unordered_map>

#include "sigslot/signal.hpp"
#include "../messages/CraftItemMessage.h"
#include "../messages/CustomPacketMessage.h"
#include "../messages/UpdateMovementMessage.h"
#include "../messages/UpdateAnimationMessage.h"
#include "../messages/UpdateAppearanceMessage.h"
#include "../messages/UpdateEquipmentMessage.h"
#include "../messages/ActivateMessage.h"
#include "../messages/PutItemMessage.h"
#include "../messages/TakeItemMessage.h"
#include "../messages/DropItemMessage.h"
#include "../messages/PlayerBowShotMessage.h"
#include "../messages/FinishSpSnippetMessage.h"
#include "../messages/OnEquipMessage.h"
#include "../messages/ConsoleCommandMessage.h"
#include "../messages/HostMessage.h"
#include "../messages/CustomEventMessage.h"
#include "../messages/ChangeValuesMessage.h"
#include "../messages/HitMessage.h"
#include "../messages/UpdateAnimVariablesMessage.h"
#include "../messages/SpellCastMessage.h"

using ProfileId = int32_t;
class ActionListener;
class MessageSerializer;
class CraftService;

class PartOneSendTargetWrapper : public Networking::ISendTarget
{
public:
  explicit PartOneSendTargetWrapper(
    Networking::ISendTarget& underlyingSendTarget_);

  void Send(Networking::UserId targetUserId, Networking::PacketData data,
            size_t length, bool reliable) override;

  void Send(Networking::UserId targetUserId, const IMessageBase& message,
            bool reliable);

private:
  Networking::ISendTarget& underlyingSendTarget;
};

class PartOneEvents
{
public:
  sigslot::signal<MessageEvent<CraftItemMessage>> onCraftItemMessage;
  sigslot::signal<MessageEvent<CustomPacketMessage>> onCustomPacketMessage;
  sigslot::signal<MessageEvent<UpdateMovementMessage>> onUpdateMovementMessage;
  sigslot::signal<MessageEvent<UpdateAnimationMessage>> onUpdateAnimationMessage;
  sigslot::signal<MessageEvent<UpdateAppearanceMessage>> onUpdateAppearanceMessage;
  sigslot::signal<MessageEvent<UpdateEquipmentMessage>> onUpdateEquipmentMessage;
  sigslot::signal<MessageEvent<ActivateMessage>> onActivateMessage;
  sigslot::signal<MessageEvent<PutItemMessage>> onPutItemMessage;
  sigslot::signal<MessageEvent<TakeItemMessage>> onTakeItemMessage;
  sigslot::signal<MessageEvent<DropItemMessage>> onDropItemMessage;
  sigslot::signal<MessageEvent<PlayerBowShotMessage>> onPlayerBowShotMessage;
  sigslot::signal<MessageEvent<FinishSpSnippetMessage>> onFinishSpSnippetMessage;
  sigslot::signal<MessageEvent<OnEquipMessage>> onOnEquipMessage;
  sigslot::signal<MessageEvent<ConsoleCommandMessage>> onConsoleCommandMessage;
  sigslot::signal<MessageEvent<HostMessage>> onHostMessage;
  sigslot::signal<MessageEvent<CustomEventMessage>> onCustomEventMessage;
  sigslot::signal<MessageEvent<ChangeValuesMessage>> onChangeValuesMessage;
  sigslot::signal<MessageEvent<HitMessage>> onHitMessage;
  sigslot::signal<MessageEvent<UpdateAnimVariablesMessage>> onUpdateAnimVariablesMessage;
  sigslot::signal<MessageEvent<SpellCastMessage>> onSpellCastMessage;
  sigslot::signal<const RawMessageData&> onUnknownMessage;
};

class PartOne : public PartOneEvents
{
public:
  struct Message
  {
    nlohmann::json j;
    std::shared_ptr<IMessageBase> message;
    Networking::UserId userId = Networking::InvalidUserId;
    bool reliable = false;
  };

  using Listener = PartOneListener;

  PartOne(Networking::ISendTarget* sendTarget = nullptr);
  PartOne(std::shared_ptr<Listener> listener,
          Networking::ISendTarget* sendTarget = nullptr);
  ~PartOne();

  void SetSendTarget(Networking::ISendTarget* sendTarget);
  void SetDamageFormula(std::unique_ptr<IDamageFormula> dmgFormula);
  void AddListener(std::shared_ptr<Listener> listener);
  bool IsConnected(Networking::UserId userId) const;
  void Tick();
  FormCallbacks CreateFormCallbacks();
  ActionListener& GetActionListener();
  const std::vector<std::shared_ptr<Listener>>& GetListeners() const;
  std::vector<Message>& Messages();

  // for CraftTest.cpp
  std::shared_ptr<CraftService> GetCraftService() const noexcept;

  // API
  uint32_t CreateActor(uint32_t formId, const NiPoint3& pos, float angleZ,
                       uint32_t cellOrWorld, ProfileId profileId = -1);
  void SetUserActor(Networking::UserId userId, uint32_t actorFormId);
  uint32_t GetUserActor(Networking::UserId userId);
  std::string GetUserGuid(Networking::UserId userId);
  Networking::UserId GetUserByActor(uint32_t formId);
  void DestroyActor(uint32_t actorFormId);
  void SetRaceMenuOpen(uint32_t formId, bool open);
  void SendCustomPacket(Networking::UserId userId,
                        const std::string& jContent);
  std::string GetActorName(uint32_t actorFormId);
  NiPoint3 GetActorPos(uint32_t actorFormId);
  uint32_t GetActorCellOrWorld(uint32_t actorFormId);
  const std::set<uint32_t>& GetActorsByProfileId(ProfileId profileId);
  void SetEnabled(uint32_t actorFormId, bool enabled);

  void AttachEspm(espm::Loader* espm);
  void AttachSaveStorage(std::shared_ptr<ISaveStorage> saveStorage);
  espm::Loader& GetEspm() const;
  bool HasEspm() const;
  void AttachLogger(std::shared_ptr<spdlog::logger> logger);
  spdlog::logger& GetLogger();

  static void HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length);

  WorldState worldState;
  ServerState serverState;
  AnimationSystem animationSystem;

  PartOneSendTargetWrapper& GetSendTarget() const;

  float CalculateDamage(const MpActor& aggressor, const MpActor& target,
                        const HitData& hitData) const;

  float CalculateDamage(const MpActor& aggressor, const MpActor& target,
                        const SpellCastData& spellCastData) const;

  void NotifyGamemodeApiStateChanged(
    const GamemodeApi::State& newState) noexcept;

  void SetPacketHistoryRecording(Networking::UserId userId, bool value);
  PacketHistory GetPacketHistory(Networking::UserId userId);
  void ClearPacketHistory(Networking::UserId userId);
  void RequestPacketHistoryPlayback(Networking::UserId userId,
                                    const PacketHistory& history);

  void SendHostStop(Networking::UserId badHosterUserId,
                    MpObjectReference& remote);

  static MessageSerializer& GetMessageSerializerInstance();

private:
  void Init();

  enum class UserType
  {
    User,
    Bot
  };

  void AddUser(Networking::UserId userId, UserType userType,
               const std::string& guid);

  void HandleMessagePacket(Networking::UserId userId,
                           Networking::PacketData data, size_t length);

  void TickPacketHistoryPlaybacks();
  void TickDeferredMessages();

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
