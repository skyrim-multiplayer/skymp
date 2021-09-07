#include <catch2/catch.hpp>
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

std::vector<MovementData> MakeTestMovementDataVector() {
  std::vector<MovementData> result;
  for (RunMode runMode : {RunMode::Running, RunMode::Sprinting, RunMode::Standing, RunMode::Walking}) {
    result.emplace_back(MakeTestMovementData(runMode, false));
    result.emplace_back(MakeTestMovementData(runMode, true));
  }
  return result;
}

}

TEST_CASE("MovementData correctly encoded and decoded to JSON",
          "[Serialization]")
{
  for (const auto& movData : MakeTestMovementDataVector()) {
    const auto json = MovementDataToJson(movData);
    const auto movData2 = MovementDataFromJson(json);
    REQUIRE(movData == movData2);
  }
}

TEST_CASE("MovementData correctly encoded and decoded to BitStream",
          "[Serialization]")
{
  for (const auto& movData : MakeTestMovementDataVector()) {
    SLNet::BitStream stream;
    Write(movData, stream);

    MovementData movData2;
    ReadTo(movData2, stream);

    REQUIRE(movData == movData2);
  }
}
