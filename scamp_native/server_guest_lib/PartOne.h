#pragma once
#include "Networking.h"
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

  static void HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length);

private:
  void HandleMessagePacket(Networking::UserId userId,
                           Networking::PacketData data, size_t length);

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};