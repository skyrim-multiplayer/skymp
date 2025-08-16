#pragma once
#include "NetworkingInterface.h" // UserId, PacketData
#include <cstdint>
#include <memory>

class PartOne;

class PacketParser
{
public:
  PacketParser();
  void TransformPacketIntoAction(Networking::UserId userId,
                                 Networking::PacketData packetData,
                                 size_t packetLength,
                                 PartOne& partOne);

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
