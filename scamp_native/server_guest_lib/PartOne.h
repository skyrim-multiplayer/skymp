#pragma once
#include "MpActor.h"
#include "Networking.h"
#include "NiPoint3.h"
#include "ServerState.h"
#include "WorldState.h"
#include <memory>
#include <simdjson.h>

class PartOne
{
public:
  class Listener
  {
  public:
    virtual ~Listener() = default;
    virtual void OnConnect(Networking::UserId userId) = 0;
    virtual void OnDisconnect(Networking::UserId userId) = 0;
    virtual void OnCustomPacket(Networking::UserId userId,
                                const simdjson::dom::element& content) = 0;
  };

  PartOne();
  PartOne(std::shared_ptr<Listener> listener);
  ~PartOne();

  void AddListener(std::shared_ptr<Listener> listener);

  bool IsConnected(Networking::UserId userId) const;

  // API
  void CreateActor(uint32_t formId, const NiPoint3& pos, float angleZ,
                   uint32_t cellOrWorld, Networking::ISendTarget* sendTarget);
  void SetUserActor(Networking::UserId userId, uint32_t actorFormId,
                    Networking::ISendTarget* sendTarget);
  uint32_t GetUserActor(Networking::UserId userId);
  void DestroyActor(uint32_t actorFormId);
  void SetRaceMenuOpen(uint32_t formId, bool open,
                       Networking::ISendTarget* sendTarget);
  void SendCustomPacket(Networking::UserId userId, const std::string& jContent,
                        Networking::ISendTarget* sendTarget);

  std::string GetActorName(uint32_t actorFormId);
  NiPoint3 GetActorPos(uint32_t actorFormId);

  Networking::UserId ConnectBot();
  void DisconnectBot(Networking::UserId id);

  static void HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length);

  WorldState worldState;
  ServerState serverState;
  Networking::ISendTarget* pushedSendTarget = nullptr;

private:
  enum class UserType
  {
    User,
    Bot
  };

  void AddUser(Networking::UserId userId, UserType userType);

  // Returns user's actor if exists
  MpActor* SendToNeighbours(const simdjson::dom::element& jMessage,
                            Networking::UserId userId,
                            Networking::PacketData data, size_t length,
                            bool reliable = false);

  void HandleMessagePacket(Networking::UserId userId,
                           Networking::PacketData data, size_t length);

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};