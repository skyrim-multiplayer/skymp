#pragma once
#include "NetworkingInterface.h"
#include <memory>
#include <utility>

namespace Networking {
class MockServer : public IServer
{
public:
  MockServer();

  std::pair<std::shared_ptr<IClient>, Networking::UserId> CreateClient();

  void Send(UserId targetUserId, PacketData data, size_t length,
            bool reliable) override;

  void Tick(OnPacket onPacket, void* state) override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
}
