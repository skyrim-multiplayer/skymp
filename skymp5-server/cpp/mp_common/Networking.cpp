#include "Networking.h"

#include <array>
#include <chrono>
#include <memory>

#include <fmt/format.h>
#include <prometheus/core.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <slikenet/MessageIdentifiers.h>
#include <slikenet/types.h>
#include <spdlog/spdlog.h>

#include "Exceptions.h"
#include "IdManager.h"
#include "NetworkingInterface.h"

namespace {
class PacketGuard
{
public:
  PacketGuard(RakPeerInterface* peer_, Packet* packet_)
    : peer(peer_)
    , packet(packet_)
  {
  }

  ~PacketGuard() { peer->DeallocatePacket(packet); }

private:
  RakPeerInterface* const peer;
  Packet* const packet;
};

const char* GetError(unsigned char packetType)
{
  switch (packetType) {
    case ID_ALREADY_CONNECTED:
      return "Already connected";
    case ID_CONNECTION_BANNED:
      return "Banned";
    case ID_INVALID_PASSWORD:
      return "Invalid password";
    case ID_INCOMPATIBLE_PROTOCOL_VERSION:
      return "Incompatible protocol version";
    case ID_IP_RECENTLY_CONNECTED:
      return "IP recently connected";
    case ID_NO_FREE_INCOMING_CONNECTIONS:
      return "No free incoming connections";
    default:
      return "";
  }
}

class Client : public Networking::IClient
{
public:
  Client(const char* ip_, unsigned short port_, int timeoutMs_,
         const char* password_)
    : ip(ip_)
    , port(port_)
    , password(password_)
  {

    peer.reset(new RakPeer);
    socket.reset(new SocketDescriptor(0, nullptr));
    const auto res = peer->Startup(1, &*socket, 1);
    if (res != StartupResult::RAKNET_STARTED) {
      throw std::runtime_error("Peer startup failed with code " +
                               std::to_string(static_cast<int>(res)));
    }
    const auto conRes = peer->Connect(ip.data(), port, password.data(),
                                      static_cast<int>(password.size()));
    if (conRes != ConnectionAttemptResult::CONNECTION_ATTEMPT_STARTED) {
      throw std::runtime_error("Peer connect failed with code " +
                               std::to_string(static_cast<int>(conRes)));
    }
    peer->SetTimeoutTime(timeoutMs_, {});
  }

  ~Client() override
  {
    packetGuard.reset(); // Depends on peer, so must be reset first
    peer->Shutdown(0);
  }

  void Send(Networking::PacketData data, size_t length, bool reliable) override
  {
    peer->Send(reinterpret_cast<const char*>(data), length, MEDIUM_PRIORITY,
               reliable ? RELIABLE : UNRELIABLE, 0, serverGuid, false);
  }

  void Tick(OnPacket onPacket, void* state) override
  {
    std::weak_ptr<SLNet::RakPeerInterface> weakPeer = peer;
    while (1) {
      Packet* packet = nullptr;
      auto p = weakPeer.lock();
      if (p) {
        packet = p->Receive();
        packetGuard.reset(new PacketGuard(&*p, packet));
      }
      p.reset();
      if (!packet)
        break;

      if (packet->data[0] == ID_CONNECTION_REQUEST_ACCEPTED) {
        serverGuid = packet->guid;
        isConnected = true;
      }

      if (packet->data[0] == ID_CONNECTION_LOST ||
          packet->data[0] == ID_DISCONNECTION_NOTIFICATION) {
        isConnected = false;
      }

      HandlePacketClientside(onPacket, state, packet);
    }
  }

  bool IsConnected() const override { return isConnected; }

private:
  const std::string ip;
  const unsigned short port;
  const std::string password;

  RakNetGUID serverGuid = UNASSIGNED_RAKNET_GUID;
  std::shared_ptr<RakPeerInterface> peer;
  std::unique_ptr<SocketDescriptor> socket;
  std::unique_ptr<PacketGuard> packetGuard;
  bool isConnected = false;
};

class Server : public Networking::IServer
{
public:
  constexpr static int timeoutTimeMs = 60000;

  Server(const char* listenAddress, unsigned short port_,
         unsigned short maxConnections_, const char* password_,
         std::shared_ptr<prometheus::Registry> promRegistry)
    : maxConnections(maxConnections_)
    , password(password_)
    , metrics{ Metrics::Init(promRegistry) }
  {
    if (maxConnections > kMaxPlayers) {
      throw std::runtime_error("Current slots limit is " +
                               std::to_string(kMaxPlayers));
    }

    idManager = std::make_unique<IdManager>(maxConnections);
    peer = std::make_unique<RakPeer>();
    socket = std::make_unique<SocketDescriptor>(port_, listenAddress);

    const auto res = peer->Startup(maxConnections, &*socket, 1);
    if (res != StartupResult::RAKNET_STARTED) {
      throw std::runtime_error("Peer startup failed with code " +
                               std::to_string(static_cast<int>(res)));
    }
    peer->SetMaximumIncomingConnections(maxConnections);
    peer->SetTimeoutTime(timeoutTimeMs, {});
    if (!password.empty()) {
      peer->SetIncomingPassword(password.data(),
                                static_cast<int>(password.size()));
    }
    peer->SetLimitIPConnectionFrequency(true);
  }

  void Send(Networking::UserId id, Networking::PacketData data, size_t length,
            bool reliable) override
  {
    const auto guid = idManager->find(id);
    if (guid == RakNetGUID(-1)) {
      throw std::runtime_error("User with id " + std::to_string(id) +
                               " doesn't exist");
    }

    peer->Send(reinterpret_cast<const char*>(data), length, MEDIUM_PRIORITY,
               reliable ? RELIABLE_ORDERED : UNRELIABLE, 0, guid, false);
  }

  void Tick(OnPacket onPacket, void* state) override
  {
    while (1) {
      auto packet = peer->Receive();
      if (!packet)
        break;
      PacketGuard guard(peer.get(), packet);
      try {
        Networking::HandlePacketServerside(onPacket, state, packet,
                                           *this->idManager);
      } catch (PublicError& e) {
        // TODO: Send PublicError to related client
        throw;
      } catch (std::exception& e) {
        throw;
      }
    }

    const auto currentTime = std::chrono::steady_clock::now();
    if (currentTime - lastMetricsUpdate > Metrics::kUpdatePeriod) {
      lastMetricsUpdate = currentTime;
      UpdateMetrics();
    }
  }

  void UpdateMetrics()
  {
    unsigned short connectedCount = 0;

    for (Networking::UserId userId = 0; userId < maxConnections; ++userId) {
      const auto guid = idManager->find(userId);
      int clientPing = -1;
      if (guid != RakNetGUID(-1)) {
        static_assert(std::is_same_v<decltype(clientPing),
                                     decltype(peer->GetLastPing(guid))>);
        clientPing = peer->GetLastPing(guid);
        connectedCount++;
      }

      auto& slotPing = metrics.pingPerSlotGaugeFamily.Add(
        { { "networking_user_id", std::to_string(userId) } });
      if (clientPing != -1) {
        metrics.overallPingSecondsHistogram.Observe(clientPing / 1000.);
        slotPing.Set(clientPing / 1000.);
      } else {
        metrics.pingPerSlotGaugeFamily.Remove(&slotPing);
      }
    }

    metrics.connectedClientsGauge.Set(connectedCount);
  }

  std::string GetIp(Networking::UserId userId) const override
  {
    const auto guid = idManager->find(userId);
    if (guid == RakNetGUID(-1)) {
      throw std::runtime_error("User with id " + std::to_string(userId) +
                               " doesn't exist");
    }

    auto address = peer->GetSystemAddressFromGuid(guid);
    std::string ip = address.ToString(false);
    return ip;
  }

  void CloseConnection(Networking::UserId userId) override
  {
    const auto guid = idManager->find(userId);
    if (guid == RakNetGUID(-1)) {
      throw std::runtime_error("User with id " + std::to_string(userId) +
                               " doesn't exist");
    }

    peer->CloseConnection(guid, true);
  }

private:
  const unsigned short maxConnections;
  const std::string password;
  std::unique_ptr<RakPeerInterface> peer;
  std::unique_ptr<SocketDescriptor> socket;
  std::unique_ptr<IdManager> idManager;

  std::chrono::time_point<std::chrono::steady_clock> lastMetricsUpdate;

  struct Metrics
  {
    std::shared_ptr<prometheus::Registry> registry;
    prometheus::Gauge<double&> connectedClientsGauge;
    prometheus::Histogram<double&> overallPingSecondsHistogram;
    prometheus::CustomFamily<prometheus::Gauge<double>>&
      pingPerSlotGaugeFamily;

    static constexpr std::chrono::seconds kUpdatePeriod{ 3 };

    static Metrics Init(std::shared_ptr<prometheus::Registry> registry)
    {
      return {
        .registry = registry,
        .connectedClientsGauge{
          registry,
          "skymp_server_connected_clients_count",
          "Count of currently conneted clients (as seen by ID manager)",
        },
        .overallPingSecondsHistogram{
          registry,
          "skymp_server_overall_ping_seconds",
          "Overview of all connected clients' ping. Converted to seconds to "
          "match Prometheus conventions",
          {},
          {
            0.025,
            0.050,
            0.075,
            0.100,
            0.125,
            0.150,
            0.175,
            0.200,
            0.250,
            0.300,
            0.400,
          },
        },
        .pingPerSlotGaugeFamily{ registry->Add<prometheus::Gauge<double>>(
          "skymp_server_ping_per_slot_seconds",
          "Last known ping for each server slot. Converted to seconds to "
          "match Prometheus conventions") },
      };
    }
  };
  Metrics metrics;
};
} // namespace

std::shared_ptr<Networking::IClient> Networking::CreateClient(
  const char* serverIp, unsigned short serverPort, int timeoutMs,
  const char* password)
{
  return std::make_shared<Client>(serverIp, serverPort, timeoutMs, password);
}

std::shared_ptr<Networking::IServer> Networking::CreateServer(
  const char* listenAddress, unsigned short port,
  unsigned short maxConnections, const char* password,
  std::shared_ptr<prometheus::Registry> promRegistry)
{
  return std::make_shared<Server>(listenAddress, port, maxConnections,
                                  password, promRegistry);
}

void Networking::HandlePacketClientside(Networking::IClient::OnPacket onPacket,
                                        void* state, Packet* packet)
{
  const auto packetId = packet->data[0];
  const auto err = GetError(packetId);
  if (packetId >= Networking::MinPacketId) {
    onPacket(state, Networking::PacketType::Message, packet->data,
             packet->length, "");
  } else if (packetId == ID_CONNECTION_LOST ||
             packetId == ID_DISCONNECTION_NOTIFICATION) {
    onPacket(state, Networking::PacketType::ClientSideDisconnect, nullptr, 0,
             "");
  } else if (packetId == ID_CONNECTION_ATTEMPT_FAILED) {
    onPacket(state, Networking::PacketType::ClientSideConnectionFailed,
             nullptr, 0, "");
  } else if (packetId == ID_CONNECTION_REQUEST_ACCEPTED) {
    onPacket(state, Networking::PacketType::ClientSideConnectionAccepted,
             nullptr, 0, "");
  } else if (err[0]) {
    onPacket(state, Networking::PacketType::ClientSideConnectionDenied,
             nullptr, 0, err);
  }
}

void Networking::HandlePacketServerside(Networking::IServer::OnPacket onPacket,
                                        void* state, Packet* packet,
                                        IdManager& idManager)
{
  const auto packetId = packet->data[0];
  Networking::UserId userId;
  switch (packetId) {
    case ID_DISCONNECTION_NOTIFICATION:
    case ID_CONNECTION_LOST:
      userId = idManager.find(packet->guid);
      if (userId == Networking::InvalidUserId) {
        throw std::runtime_error(fmt::format(
          "Unexpected disconnection for system without userId (guid={})",
          packet->guid.g));
      }
      onPacket(state, userId, Networking::PacketType::ServerSideUserDisconnect,
               nullptr, 0);
      idManager.freeId(userId);
      break;
    case ID_NEW_INCOMING_CONNECTION: {
      userId = idManager.allocateId(packet->guid);
      if (userId == Networking::InvalidUserId) {
        throw std::runtime_error("idManager is full");
      }

      std::array<char, 256> guidToStringDestination;
      packet->guid.ToString(guidToStringDestination.data(),
                            std::size(guidToStringDestination));
      std::string guid = guidToStringDestination.data();

      onPacket(state, userId, Networking::PacketType::ServerSideUserConnect,
               reinterpret_cast<PacketData>(guid.data()), guid.size());

      break;
    }
    default:
      userId = idManager.find(packet->guid);
      if (packetId >= Networking::MinPacketId) {
        onPacket(state, userId, Networking::PacketType::Message, packet->data,
                 packet->length);
      }
      break;
  }
}
