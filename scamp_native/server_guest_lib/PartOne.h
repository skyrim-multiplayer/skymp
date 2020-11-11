#pragma once
#include "ISaveStorage.h"
#include "MpActor.h"
#include "Networking.h"
#include "NiPoint3.h"
#include "ServerState.h"
#include "WorldState.h"
#include <Loader.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <simdjson.h>
#include <spdlog/logger.h>
#include <unordered_map>

using ProfileId = int32_t;

class IActionListener;

class PartOne
{
public:
  struct Message
  {
    nlohmann::json j;
    Networking::UserId userId = Networking::InvalidUserId;
    bool reliable = false;
  };

  class Listener
  {
  public:
    virtual ~Listener() = default;
    virtual void OnConnect(Networking::UserId userId) = 0;
    virtual void OnDisconnect(Networking::UserId userId) = 0;
    virtual void OnCustomPacket(Networking::UserId userId,
                                const simdjson::dom::element& content) = 0;
  };

  PartOne(Networking::ISendTarget* sendTarget = nullptr);
  PartOne(std::shared_ptr<Listener> listener,
          Networking::ISendTarget* sendTarget = nullptr);
  ~PartOne();

  void SetSendTarget(Networking::ISendTarget* sendTarget);
  void AddListener(std::shared_ptr<Listener> listener);
  bool IsConnected(Networking::UserId userId) const;
  void Tick();
  void EnableProductionHacks();
  FormCallbacks CreateFormCallbacks();
  IActionListener& GetActionListener();
  const std::vector<std::shared_ptr<Listener>>& GetListeners() const;
  std::vector<Message>& Messages();

  // API
  uint32_t CreateActor(uint32_t formId, const NiPoint3& pos, float angleZ,
                       uint32_t cellOrWorld, ProfileId profileId = -1);
  void SetUserActor(Networking::UserId userId, uint32_t actorFormId);
  uint32_t GetUserActor(Networking::UserId userId);
  void DestroyActor(uint32_t actorFormId);
  void SetRaceMenuOpen(uint32_t formId, bool open);
  void SendCustomPacket(Networking::UserId userId,
                        const std::string& jContent);
  std::string GetActorName(uint32_t actorFormId);
  NiPoint3 GetActorPos(uint32_t actorFormId);
  const std::set<uint32_t>& GetActorsByProfileId(ProfileId profileId);
  void SetEnabled(uint32_t actorFormId, bool enabled);

  void AttachEspm(espm::Loader* espm);
  void AttachSaveStorage(std::shared_ptr<ISaveStorage> saveStorage);
  espm::Loader& GetEspm() const;
  bool HasEspm() const;
  void AttachLogger(std::shared_ptr<spdlog::logger> logger);

  static void HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length);

  WorldState worldState;
  ServerState serverState;

  Networking::ISendTarget& GetSendTarget() const;

private:
  void Init();

  enum class UserType
  {
    User,
    Bot
  };

  void AddUser(Networking::UserId userId, UserType userType);

  void HandleMessagePacket(Networking::UserId userId,
                           Networking::PacketData data, size_t length);

  void InitActionListener();

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};