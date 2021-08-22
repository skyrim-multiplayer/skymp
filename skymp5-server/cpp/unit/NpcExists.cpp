#include "TestUtils.hpp"
#include <catch2/catch.hpp>

PartOne& GetPartOne();

TEST_CASE("Whiterun cow 0x10ebaf exists", "[NpcExists][espm]")
{
  auto& partOne = GetPartOne();

  auto& cow = partOne.worldState.GetFormAt<MpActor>(0x10ebaf);
}

TEST_CASE("Whiterun cow 0x10ebaf is visible to players", "[NpcExists][espm]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 24132, -5049, -3004 }, 0, 0x1a26f);

  partOne.SetUserActor(0, 0xff000000);

  auto messages = partOne.Messages();

  {
    auto it = std::find_if(
      messages.begin(), messages.end(),
      [&](PartOne::Message msg) { return msg.j["refrId"] == 0x10010ebaf; });

    REQUIRE(it != messages.end());
  }

  {
    auto it = std::find_if(messages.begin(), messages.end(),
                           [&](PartOne::Message msg) {
                             return msg.j["refrId"] == 0x10010ebaf &&
                               (msg.j["isHostedByOther"] == false ||
                                msg.j["isHostedByOther"] == nullptr);
                           });

    REQUIRE(it != messages.end());
  }
}
