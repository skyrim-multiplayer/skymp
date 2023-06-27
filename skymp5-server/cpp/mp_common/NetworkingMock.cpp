#include "NetworkingMock.h"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace NetworkingMock {
struct Packet
{
  Networking::PacketType type = Networking::PacketType::Invalid;
  std::vector<uint8_t> data;
  const char* error = "";
};
}

namespace NetworkingMock {
class MockClient;

typedef void (*SendFn)(Networking::MockServer*, Networking::UserId id,
                       Networking::PacketType type,
                       Networking::PacketData data, size_t length,
                       bool reliable);

class MockClient : public Networking::IClient
{
public:
  MockClient(Networking::MockServer* parent_, SendFn sendFn_)
    : parent(parent_)
    , sendFn(sendFn_)
  {
  }

  void AddPacket(std::unique_ptr<NetworkingMock::Packet> p)
  {
    packets.push_back(std::move(p));
  }

  void Send(Networking::PacketData data, size_t length, bool reliable) override
  {
    return sendFn(parent, id, Networking::PacketType::Message, data, length,
                  reliable);
  }

  void Tick(OnPacket onPacket, void* state) override
  {
    for (auto& p : packets)
      onPacket(state, p->type, p->data.empty() ? nullptr : &p->data[0],
               p->data.size(), p->error);
    packets.clear();
  }

  bool IsConnected() const override { return true; }

  ~MockClient() override
  {
    sendFn(parent, id, Networking::PacketType::ServerSideUserDisconnect,
           nullptr, 0, true);
  }

  void SetId(Networking::UserId userId) { id = userId; }

private:
  Networking::MockServer* const parent;
  const SendFn sendFn;
  std::vector<std::unique_ptr<NetworkingMock::Packet>> packets;
  Networking::UserId id = Networking::InvalidUserId;
};
}

struct Networking::MockServer::Impl
{
  std::vector<std::weak_ptr<NetworkingMock::MockClient>> clients;
  std::vector<
    std::pair<Networking::UserId, std::unique_ptr<NetworkingMock::Packet>>>
    packets;
};

Networking::MockServer::MockServer()
{
  pImpl.reset(new Impl);
}

std::pair<std::shared_ptr<Networking::IClient>, Networking::UserId>
Networking::MockServer::CreateClient()
{
  NetworkingMock::SendFn sendFn =
    [](Networking::MockServer* parent, Networking::UserId id,
       Networking::PacketType type, Networking::PacketData data, size_t length,
       bool reliable) {
      parent->pImpl->packets.push_back(
        { id,
          std::unique_ptr<NetworkingMock::Packet>(new NetworkingMock::Packet(
            { type,
              data ? std::vector<uint8_t>(data, data + length)
                   : std::vector<uint8_t>() })) });
    };

  auto it =
    std::find_if(pImpl->clients.begin(), pImpl->clients.end(),
                 [&](const std::weak_ptr<NetworkingMock::MockClient>& cl) {
                   return cl.expired() || !cl.lock().get();
                 });

  auto cl = std::make_shared<NetworkingMock::MockClient>(this, sendFn);

  Networking::UserId myId;
  if (it != pImpl->clients.end()) {
    myId = it - pImpl->clients.begin();
    *it = cl;
  } else {
    myId = pImpl->clients.size();
    pImpl->clients.push_back(cl);
  }

  cl->SetId(myId);

  pImpl->packets.push_back(
    { myId,
      std::unique_ptr<NetworkingMock::Packet>(new NetworkingMock::Packet(
        { Networking::PacketType::ServerSideUserConnect })) });

  cl->AddPacket(
    std::unique_ptr<NetworkingMock::Packet>(new NetworkingMock::Packet(
      { Networking::PacketType::ClientSideConnectionAccepted })));

  return { cl, myId };
}

void Networking::MockServer::Send(UserId targetUserId, PacketData data,
                                  size_t length, bool reliable)
{
  if (pImpl->clients.size() <= targetUserId ||
      pImpl->clients[targetUserId].expired() ||
      !pImpl->clients[targetUserId].lock())
    throw std::runtime_error("No client with id " +
                             std::to_string(targetUserId) +
                             " found on MockServer");
  auto cl = pImpl->clients[targetUserId].lock();
  cl->AddPacket(
    std::unique_ptr<NetworkingMock::Packet>(new NetworkingMock::Packet(
      { Networking::PacketType::Message,
        std::vector<uint8_t>(data, data + length) })));
}

void Networking::MockServer::Tick(OnPacket onPacket, void* state)
{
  for (auto& pair : pImpl->packets) {
    auto& p = pair.second;
    onPacket(state, pair.first, p->type,
             p->data.empty() ? nullptr : &p->data[0], p->data.size());
  }
  pImpl->packets.clear();
}
