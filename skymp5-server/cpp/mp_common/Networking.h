#pragma once
#include "NetworkingInterface.h"
#include "RakNet.h"
#include <cstdlib>
#include <memory>
#include <vector>

class IdManager;

namespace Networking {

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
