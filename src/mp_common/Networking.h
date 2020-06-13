#pragma once
#include "RakNet.h"
#include <memory>

class IdManager;

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
  typedef void (*OnPacket)(void* state, PacketType packetType, PacketData data,
                           size_t length, const char* error);

  virtual ~IClient() = default;

  virtual void Send(PacketData data, size_t length, bool reliable) = 0;
  virtual void Tick(OnPacket onPacket, void* state) = 0;
  virtual bool IsConnected() const = 0;
};

class IServer
{
public:
  typedef void (*OnPacket)(void* state, UserId userId, PacketType packetType,
                           PacketData data, size_t length);

  virtual ~IServer() = default;

  virtual void Send(UserId targetUserId, PacketData data, size_t length,
                    bool reliable) = 0;

  virtual void Tick(OnPacket onPacket, void* state) = 0;
};

std::shared_ptr<IClient> CreateClient(const char* serverIp,
                                      unsigned short serverPort,
                                      int timeoutMs = 4000);
std::shared_ptr<IServer> CreateServer(unsigned short port,
                                      unsigned short maxConnections);

void HandlePacketClientside(Networking::IClient::OnPacket onPacket,
                            void* state, Packet* packet);

void HandlePacketServerside(Networking::IServer::OnPacket onPacket,
                            void* state, Packet* packet, IdManager& idManager);
}