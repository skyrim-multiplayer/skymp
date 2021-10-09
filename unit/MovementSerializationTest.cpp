#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

#include "MovementData.h"
#include "MovementDataSerialization.h"

namespace {

MovementData MakeTestMovementData(RunMode runMode, bool hasLookAt) {
  MovementData result{
    1337, // idx
    0x2077, // worldOrCell
    { 0.25, -100, 0 }, // pos
    { 123, 0, 45 }, // rot
    270, // direction
    0.5, // healthPercentage
    RunMode::Running,
    true, // isInJumpState
    true, // isSneaking
    false, // isBlocking
    true, // isWeapDrawn
    {{ 1, 2, 3 }}, // lookAt
  };
  result.runMode = runMode;
  if (!hasLookAt) {
    result.lookAt = std::nullopt;
  }
  return result;
}

std::unordered_map<std::string, MovementData> MakeTestMovementDataCases() {
  std::unordered_map<std::string, MovementData> result;
  for (RunMode runMode : {RunMode::Running, RunMode::Sprinting, RunMode::Standing, RunMode::Walking}) {
    result.emplace(fmt::format("{},{}", ToString(runMode), false), MakeTestMovementData(runMode, false));
    result.emplace(fmt::format("{},{}", ToString(runMode), false), MakeTestMovementData(runMode, true));
  }
  return result;
}

}

TEST_CASE("MovementData correctly encoded and decoded to JSON",
          "[Serialization]")
{
  for (const auto& [name, movData] : MakeTestMovementDataCases()) {
    SECTION(name) {
      const auto json = MovementDataToJson(movData);
      const auto movData2 = MovementDataFromJson(json);
      REQUIRE(movData == movData2);
    }
  }
}

TEST_CASE("MovementData correctly encoded and decoded to BitStream",
          "[Serialization]")
{
  for (const auto& [name, movData] : MakeTestMovementDataCases()) {
    SECTION(name) {
      SLNet::BitStream stream;
      WriteToBitStream(stream, movData);

      MovementData movData2;
      ReadFromBitStream(stream, movData2);

      REQUIRE(movData == movData2);
    }
  }
}
