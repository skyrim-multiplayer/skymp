#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

PartOne& GetPartOne();

static constexpr uint32_t cowId = 0x10ebaf;
static constexpr uint64_t cowRefrId = 0x10010ebaf;

TEST_CASE("Whiterun cow 0x10ebaf exists", "[NpcExists][espm]")
{
  auto& partOne = GetPartOne();
  partOne.worldState.npcEnabled = true;
  auto& cow = partOne.worldState.GetFormAt<MpActor>(cowId);
}

TEST_CASE("Whiterun cow 0x10ebaf is visible to players", "[NpcExists][espm]")
{
  uint32_t cell = ToUnderlying(Constants::kWhiterunCell);
  auto& partOne = GetPartOne();
  partOne.worldState.npcEnabled = true;
  auto& messages = partOne.Messages();

  auto& cow = partOne.worldState.GetFormAt<MpActor>(cowId);

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, cow.GetPos(), 0, cell);
  partOne.SetUserActor(0, 0xff000000);

  {
    auto it = std::find_if(
      messages.begin(), messages.end(),
      [&](PartOne::Message& msg) { return msg.j["refrId"] == cowRefrId; });

    REQUIRE(it != messages.end());
  }

  {
    auto it = std::find_if(messages.begin(), messages.end(),
                           [&](PartOne::Message& msg) {
                             return msg.j["refrId"] == cowRefrId &&
                               (msg.j["isHostedByOther"] == nullptr ||
                                msg.j["isHostedByOther"] == false);
                           });
    REQUIRE(it != messages.end());
  }

  partOne.DestroyActor(0xff000000);
  partOne.DestroyActor(cowId);
  DoDisconnect(partOne, 0);
}
