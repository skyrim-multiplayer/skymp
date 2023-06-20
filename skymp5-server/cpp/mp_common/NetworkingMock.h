#pragma once
#include "NetworkingInterface.h"
#include <memory>

namespace Networking {
class MockServer : public IServer
{
public:
  MockServer();

  std::shared_ptr<IClient> CreateClient();

  void Send(UserId targetUserId, PacketData data, size_t length,
            Networking::Reliability reliability) override;

  void Tick(OnPacket onPacket, void* state) override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
}
