#pragma once
#include <cstddef>
#include <cstdio>
#include <vector>

namespace Networking {
using UserId = unsigned short;
enum : UserId
{
  InvalidUserId = (UserId)~0
};

enum : unsigned char
{
  MinPacketId = 134
};
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
};

template <class FormatCallback, class... Ts>
inline void Format(const FormatCallback& cb, const char* format, Ts... args)
{
  auto textSize = (size_t)snprintf(nullptr, 0, format, args...);

  const auto n = textSize + sizeof('\0') + sizeof(MinPacketId);
  std::vector<char> buf(n);

  buf[0] = MinPacketId;
  auto len = (size_t)snprintf(buf.data() + 1, n - 1, format, args...);

  cb(reinterpret_cast<Networking::PacketData>(buf.data()), len + 1);
}

template <class... Ts>
inline void SendFormatted(Networking::ISendTarget* sendTarget,
                          Networking::UserId userId, const char* format,
                          Ts... args)
{
  Format([&](Networking::PacketData data,
             size_t length) { sendTarget->Send(userId, data, length, true); },
         format, args...);
}
}
