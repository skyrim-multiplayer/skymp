#include <catch2/catch_all.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

#include "MovementMessage.h"

namespace {

MovementMessage MakeTestMovementMessage(RunMode runMode, bool hasLookAt)
{
  MovementMessage result;
  result.idx = 1337;
  result.worldOrCell = 0x2077;
  result.pos = { 0.25, -100, 0 };
  result.rot = { 123, 0, 45 };
  result.direction = 270;
  result.healthPercentage = 0.5;
  result.speed = 400;
  result.runMode = RunMode::Running;
  result.isInJumpState = true;
  result.isSneaking = true;
  result.isBlocking = false;
  result.isWeapDrawn = true;
  result.isDead = false;
  result.lookAt = {{1,2,3}};

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
      nlohmann::json json;
      movData.WriteJson(json);

      MovementMessage movData2;
      movData2.ReadJson(json);

      nlohmann::json json2;
      movData2.WriteJson(json2);

      INFO(json.dump());
      REQUIRE(json == json2);
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
      movData.WriteBinary(stream);

      MovementMessage movData2;
      movData2.ReadBinary(stream);

      SLNet::BitStream stream2;
      movData2.WriteBinary(stream2);

      REQUIRE(stream.GetNumberOfBytesUsed() == stream2.GetNumberOfBytesUsed());
      REQUIRE(memcmp(stream.GetData(), stream2.GetData(), stream.GetNumberOfBytesUsed()) == 0);
    }
  }
}
