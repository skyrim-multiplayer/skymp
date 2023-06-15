#pragma once
#include <cstdint>
#include <functional>

class MpObjectReference;
class MpActor;

class FormCallbacks
{
public:
  using SubscribeCallback =
    std::function<void(uint32_t emitterFormId, uint32_t listenerFormId)>;
  using SendToUserFn = std::function<void(uint32_t formId, const void* data,
                                          size_t size, bool reliable)>;

public:
  static FormCallbacks DoNothing() noexcept;

public:
  SubscribeCallback subscribe, unsubscribe;
  SendToUserFn sendToUser;
};


struct listener
{
};

struct PacketData
{
  int minPacketId;
  int someting_else;
};

struct PacketType final : public PacketData
{
  std::string additional_info;
};
