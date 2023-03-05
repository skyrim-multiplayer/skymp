#include "PartOne.h"
#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <chrono>
#include <iostream>

class EmptySendTarget : public Networking::ISendTarget
{
public:
  void Send(Networking::UserId targetUserId, Networking::PacketData data,
            size_t length, bool reliable) override
  {
  }
};

int n = 10;

std::string GetTimePassed(std::chrono::system_clock::time_point was)
{
  auto count = std::chrono::duration_cast<std::chrono::microseconds>(
                 std::chrono::system_clock::now() - was)
                 .count() /
    n;
  return std::to_string(count) + " microseconds";
}

PartOne& GetPartOne();

void ExecuteBenchmark(int numPlayers)
{
  PartOne& p = GetPartOne();
  p.SetSendTarget(new EmptySendTarget);

  REQUIRE(kMaxPlayers >= numPlayers);
  for (int i = 0; i < numPlayers; ++i) {
    DoConnect(p, i);
    p.CreateActor(0xff000000 + i, { 0, 0, 0 }, 1, 0x3c);
    p.SetUserActor(i, 0xff000000 + i);
  }

  auto was = std::chrono::system_clock::now();

  for (int i = 0; i < n; ++i) {
    DoMessage(p, 0, jMovement);
  }

  std::cout << "DoMessage for " << numPlayers << " users took "
            << GetTimePassed(was) << std::endl;
}

TEST_CASE("SendToNeighbours", "[Benchmarks]")
{
  ExecuteBenchmark(1);
  // ExecuteBenchmark(50);
  // ExecuteBenchmark(100);
  // ExecuteBenchmark(200);
#ifdef NDEBUG
  // ExecuteBenchmark(500);
  // ExecuteBenchmark(1000);
#endif
}
