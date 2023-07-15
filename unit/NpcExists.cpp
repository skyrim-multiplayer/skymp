#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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

TEST_CASE("Whiterun cow is disallowed to be loaded because of overriden npc "
          "settings, provided enabled npcs",
          "[NpcExists][espm]")
{
  PartOne& partOne = GetPartOne();
  partOne.worldState.npcEnabled = true;
  std::unordered_map<std::string, WorldState::NpcSettingsEntry> settings = {
    { "DragonBorn.esm", { true, true } }
  };
  partOne.worldState.SetNpcSettings(std::move(settings));

  REQUIRE_THROWS_WITH(partOne.worldState.GetFormAt<MpActor>(cowId),
                      Catch::Matchers::ContainsSubstring("Form with id"));
}

TEST_CASE(
  "Whiterun cow is not loaded because only npcs within interior are allowed",
  "[NpcExists][espm]")
{
  PartOne& partOne = GetPartOne();
  partOne.worldState.npcEnabled = true;
  bool spawnInInterior = true, spawnInExterior = false;
  std::unordered_map<std::string, WorldState::NpcSettingsEntry> settings = {
    { "Skyrim.esm", { spawnInInterior, spawnInExterior } }
  };
  partOne.worldState.SetNpcSettings(std::move(settings));

  REQUIRE_THROWS_WITH(partOne.worldState.GetFormAt<MpActor>(cowId),
                      Catch::Matchers::ContainsSubstring("Form with id"));
}

TEST_CASE("Whiterun cow is successfully loaded, because only exterior npc "
          "from Skyrim.esm are allowed",
          "[NpcExists][espm]")
{
  PartOne& partOne = GetPartOne();
  partOne.worldState.npcEnabled = true;
  bool spawnInInterior = false, spawnInExterior = true;
  std::unordered_map<std::string, WorldState::NpcSettingsEntry> settings = {
    { "Skyrim.esm", { spawnInInterior, spawnInExterior } }
  };
  partOne.worldState.SetNpcSettings(std::move(settings));

  REQUIRE_NOTHROW(partOne.worldState.GetFormAt<MpActor>(cowId));
}

TEST_CASE(
  "Whiterun cow is loaded because all npc are allowed by default setting",
  "[NpcExists][espm]")
{
  PartOne& partOne = GetPartOne();
  partOne.worldState.npcEnabled = true;
  partOne.worldState.defaultSetting.spawnInInterior = true;
  partOne.worldState.defaultSetting.spawnInExterior = true;
  partOne.worldState.defaultSetting.overriden = true;
  REQUIRE_NOTHROW(partOne.worldState.GetFormAt<MpActor>(cowId));
}
