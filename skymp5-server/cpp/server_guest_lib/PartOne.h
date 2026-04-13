#pragma once
#include "AnimationSystem.h"
#include "FormDesc.h"
#include "GamemodeApi.h"
#include "HitData.h"
#include "MpActor.h"
#include "MpChangeForms.h"
#include "NiPoint3.h"
#include "PartOneListener.h"
#include "ServerState.h"
#include "SpellCastData.h"
#include "WorldState.h"
#include "formulas/IDamageFormula.h"
#include "libespm/Loader.h"
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <save_storages/ISaveStorage.h>
#include <set>
#include <simdjson.h>
#include <spdlog/logger.h>

using ProfileId = int32_t;
class ActionListener;
class MessageSerializer;

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

class PartOne
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

  using OnActorStreamIn = std::function<void(const MpActor& emitter,
                                             const MpObjectReference& listener,
                                             CreateActorMessage& message)>;
  void SetOnActorStreamIn(OnActorStreamIn callback);

  void AttachEspm(espm::Loader* espm);
  void AttachSaveStorage(
    std::shared_ptr<Viet::ISaveStorage<MpChangeForm, FormDesc>> saveStorage);
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

  void SetPrivateKey(const std::string& keyId, const std::string& pkeyPem);

  void EnableGamemodeDataUpdatesBroadcast(bool enable);

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

  void InitActionListener();

  void TickPacketHistoryPlaybacks();
  void TickDeferredMessages();

  std::string SignJavaScriptSources(const std::string& src) const;

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
