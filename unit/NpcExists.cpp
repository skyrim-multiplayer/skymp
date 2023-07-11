#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

PartOne& GetPartOne();

static constexpr uint32_t cowId = 0x10ebaf;

TEST_CASE("Whiterun cow 0x10ebaf exists", "[NpcExists][espm]")
{
  auto& partOne = GetPartOne();
  partOne.worldState.npcEnabled = true;
  auto& cow = partOne.worldState.GetFormAt<MpActor>(cowId);
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
      [&](PartOne::Message msg) { return msg.j["refrId"] == cowId; });

    REQUIRE(it != messages.end());
  }

  {
    auto it = std::find_if(messages.begin(), messages.end(),
                           [&](PartOne::Message msg) {
                             return msg.j["refrId"] == cowId &&
                               (msg.j["isHostedByOther"] == false ||
                                msg.j["isHostedByOther"] == nullptr);
                           });

    REQUIRE(it != messages.end());
  }
}
