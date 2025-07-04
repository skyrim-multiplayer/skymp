#pragma once
#include "MinPacketId.h"
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

namespace Networking {

using UserId = unsigned short;

constexpr UserId InvalidUserId = static_cast<UserId>(~0);

// First byte must represent id of the packet (>= MinPacketId)
using PacketData = const unsigned char*;

enum class PacketType
{
  Invalid = -1,
  Message,
  ClientSideDisconnect,
  ClientSideConnectionAccepted,
  ClientSideConnectionFailed,
  ClientSideConnectionDenied,
  ServerSideUserConnect,
  ServerSideUserDisconnect
};

class IClient
{
public:
  using OnPacket = void (*)(void* state, PacketType packetType,
                            PacketData data, size_t length, const char* error);

  virtual ~IClient() = default;

  virtual void Send(PacketData data, size_t length, bool reliable) = 0;
  virtual void Tick(OnPacket onPacket, void* state) = 0;
  virtual bool IsConnected() const = 0;
};

class ISendTarget
{
public:
  virtual ~ISendTarget() = default;

  virtual void Send(UserId targetUserId, PacketData data, size_t length,
                    bool reliable) = 0;
};

class IServer : public ISendTarget
{
public:
  typedef void (*OnPacket)(void* state, UserId userId, PacketType packetType,
                           PacketData data, size_t length);

  virtual ~IServer() = default;

  virtual void Tick(OnPacket onPacket, void* state) = 0;

  virtual std::string GetIp(UserId userId) const = 0;
};
}
