#include <catch2/catch_all.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

#include "MovementMessage.h"
#include "MovementMessageSerialization.h"

namespace {

MovementMessage MakeTestMovementMessage(RunMode runMode, bool hasLookAt)
{
  MovementMessage result{
    1337,              // idx
    0x2077,            // worldOrCell
    { 0.25, -100, 0 }, // pos
    { 123, 0, 45 },    // rot
    270,               // direction
    0.5,               // healthPercentage
    400,               // speed
    RunMode::Running,
    true,            // isInJumpState
    true,            // isSneaking
    false,           // isBlocking
    true,            // isWeapDrawn
    false,           // isDead
    { { 1, 2, 3 } }, // lookAt
  };
  result.runMode = runMode;
  if (!hasLookAt) {
    result.lookAt = std::nullopt;
  }
  return result;
}

std::unordered_map<std::string, MovementMessage> MakeTestMovementMessageCases()
{
  std::unordered_map<std::string, MovementMessage> result;
  for (RunMode runMode : { RunMode::Running, RunMode::Sprinting,
                           RunMode::Standing, RunMode::Walking }) {
    result.emplace(fmt::format("{},{}", ToString(runMode), false),
                   MakeTestMovementMessage(runMode, false));
    result.emplace(fmt::format("{},{}", ToString(runMode), false),
                   MakeTestMovementMessage(runMode, true));
  }
  return result;
}

}

TEST_CASE("MovementMessage correctly encoded and decoded to JSON",
          "[Serialization]")
{
  for (const auto& [name, movData] : MakeTestMovementMessageCases()) {
    SECTION(name)
    {
      const auto json = serialization::MovementMessageToJson(movData);
      const auto movData2 = serialization::MovementMessageFromJson(json);
      INFO(json.dump());
      REQUIRE(movData == movData2);
    }
  }
}

TEST_CASE("MovementMessage correctly encoded and decoded to BitStream",
          "[Serialization]")
{
  for (const auto& [name, movData] : MakeTestMovementMessageCases()) {
    SECTION(name)
    {
      SLNet::BitStream stream;
      serialization::WriteToBitStream(stream, movData);

      MovementMessage movData2;
      serialization::ReadFromBitStream(stream, movData2);

      REQUIRE(movData == movData2);
    }
  }
}
