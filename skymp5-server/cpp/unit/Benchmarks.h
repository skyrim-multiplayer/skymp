#include "PartOne.h"
#include "TestUtils.hpp"
#include <catch2/catch.hpp>
#include <chrono>
#include <iostream>

class EmptySendTarget : public Networking::ISendTarget
{
public:
  void Send(Networking::UserId targetUserId, Networking::PacketData data,
            size_t length, bool reliable) override;
};

std::string GetTimePassed(std::chrono::system_clock::time_point was);

PartOne& GetPartOne();

void ExecuteBenchmark(int numPlayers);


