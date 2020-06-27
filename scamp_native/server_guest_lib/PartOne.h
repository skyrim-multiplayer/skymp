#pragma once
#include "Networking.h"
#include "NiPoint3.h"
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

  void AddListener(std::shared_ptr<Listener> listener);

  bool IsConnected(Networking::UserId userId) const;

  // API
  void CreateActor(uint32_t formId, const NiPoint3& pos, float angleZ,
                   uint32_t cellOrWorld, Networking::IServer* svr);
  void SetUserActor(Networking::UserId userId, uint32_t actorFormId,
                    Networking::IServer* svr);
  uint32_t GetUserActor(Networking::UserId userId);
  void DestroyActor(uint32_t actorFormId);

  static void HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length);

  void PushServer(Networking::IServer* server);

private:
  Networking::IServer* PopServer();

  void HandleMessagePacket(Networking::UserId userId,
                           Networking::PacketData data, size_t length);

  struct Impl;
  std::shared_ptr<Impl> pImpl;

  Networking::IServer* pushedServer = nullptr;
};