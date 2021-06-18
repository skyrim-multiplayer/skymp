#pragma once
#include "NetworkingInterface.h"
#include "RakNet.h"
#include <cstdlib>
#include <memory>
#include <vector>

class IdManager;

namespace Networking {

/// Create new client instance
/// \param serverIp Server host or ip
/// \param serverPort Server port
/// \param timeoutMs Max timeout
/// \return Pointer to new client
std::shared_ptr<IClient> CreateClient(const char* serverIp,
                                      unsigned short serverPort,
                                      int timeoutMs = 4000);

/// Create new server instance
/// \param port Server port
/// \param maxConnections Max players num
/// \return Pointer to new server
std::shared_ptr<IServer> CreateServer(unsigned short port,
                                      unsigned short maxConnections);

/// Handle received packet on client
/// \param onPacket OnPacket callback
/// \param state Current client state
/// \param packet Packet ptr
void HandlePacketClientside(Networking::IClient::OnPacket onPacket,
                            void* state, Packet* packet);

/// Handle received packet on server
/// \param onPacket OnPacket callback
/// \param state Current server state
/// \param packet Packet ptr
/// \param idManager Current IdManager
void HandlePacketServerside(Networking::IServer::OnPacket onPacket,
                            void* state, Packet* packet, IdManager& idManager);
}