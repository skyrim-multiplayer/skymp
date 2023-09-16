#include "MovementMessage.h"
#include "SerializationUtil/BitStreamUtil.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

namespace {
const inline std::string kStanding = "Standing";
const inline std::string kWalking = "Walking";
const inline std::string kRunning = "Running";
const inline std::string kSprinting = "Sprinting";
}

void MovementMessage::WriteBinary(SLNet::BitStream& stream) const
{
  using SerializationUtil::WriteToBitStream;

  WriteToBitStream(stream, idx);
  WriteToBitStream(stream, worldOrCell);
  WriteToBitStream(stream, pos);
  WriteToBitStream(stream, rot);
  WriteToBitStream(stream, direction);
  WriteToBitStream(stream, healthPercentage);
  WriteToBitStream(stream, speed);

  WriteToBitStream(
    stream, static_cast<bool>(static_cast<uint8_t>(runMode) & 2));
  WriteToBitStream(
    stream, static_cast<bool>(static_cast<uint8_t>(runMode) & 1));

  WriteToBitStream(stream, isInJumpState);
  WriteToBitStream(stream, isSneaking);
  WriteToBitStream(stream, isBlocking);
  WriteToBitStream(stream, isWeapDrawn);
  WriteToBitStream(stream, isDead);
  WriteToBitStream(stream, lookAt);
}

void MovementMessage::ReadBinary(SLNet::BitStream& stream)
{
  using SerializationUtil::ReadFromBitStream;

  ReadFromBitStream(stream, idx);
  ReadFromBitStream(stream, worldOrCell);
  ReadFromBitStream(stream, pos);
  ReadFromBitStream(stream, rot);
  ReadFromBitStream(stream, direction);
  ReadFromBitStream(stream, healthPercentage);
  ReadFromBitStream(stream, speed);

  uint8_t tmp = 0;
  tmp |= static_cast<uint8_t>(ReadFromBitStream<bool>(stream));
  tmp <<= 1;
  tmp |= static_cast<uint8_t>(ReadFromBitStream<bool>(stream));
  runMode = static_cast<RunMode>(tmp);

  ReadFromBitStream(stream, isInJumpState);
  ReadFromBitStream(stream, isSneaking);
  ReadFromBitStream(stream, isBlocking);
  ReadFromBitStream(stream, isWeapDrawn);
  ReadFromBitStream(stream, isDead);
  ReadFromBitStream(stream, lookAt);
}

void MovementMessage::WriteJson(nlohmann::json &json) const
{
  auto result = nlohmann::json{
    { "t", MsgType::UpdateMovement },
    { "idx", idx },
    {
      "data",
      {
        { "worldOrCell", worldOrCell },
        { "pos", pos },
        { "rot", rot },
        { "runMode", ToString(runMode) },
        { "direction", direction },
        { "healthPercentage", healthPercentage },
        { "speed", speed },
        { "isInJumpState", isInJumpState },
        { "isSneaking", isSneaking },
        { "isBlocking", isBlocking },
        { "isWeapDrawn", isWeapDrawn },
        { "isDead", isDead },
      },
    },
  };
  if (lookAt) {
    result["data"]["lookAt"] = *lookAt;
  }
  json = std::move(result);
}

void MovementMessage::ReadJson(const nlohmann::json &json)
{
  MovementMessage result;

  result.idx = json.at("idx").get<uint32_t>();

  const auto& data = json.at("data");
  result.worldOrCell = data.at("worldOrCell").get<uint32_t>();
  result.pos = data.at("pos").get<std::array<float, 3>>();
  result.rot = data.at("rot").get<std::array<float, 3>>();
  result.direction = data.at("direction").get<float>();
  result.healthPercentage = data.at("healthPercentage").get<float>();
  result.speed = data.at("speed").get<float>();
  result.runMode = RunModeFromString(data.at("runMode").get<std::string_view>());
  result.isInJumpState = data.at("isInJumpState").get<bool>();
  result.isSneaking = data.at("isSneaking").get<bool>();
  result.isBlocking = data.at("isBlocking").get<bool>();
  result.isWeapDrawn = data.at("isWeapDrawn").get<bool>();
  result.isDead = data.at("isDead").get<bool>();
  const auto lookAtIt = data.find("lookAt");
  if (lookAtIt != data.end()) {
    result.lookAt = lookAtIt->get<std::array<float, 3>>();
  }

  *this = std::move(result);
}

const std::string& ToString(RunMode runMode)
{
  switch (runMode) {
    case RunMode::Standing:
      return kStanding;
    case RunMode::Walking:
      return kWalking;
    case RunMode::Running:
      return kRunning;
    case RunMode::Sprinting:
      return kSprinting;
    default:
      throw std::runtime_error("unhandled case for RunMode");
  }
}

RunMode RunModeFromString(std::string_view str)
{
  if (str == kStanding) {
    return RunMode::Standing;
  } else if (str == kWalking) {
    return RunMode::Walking;
  } else if (str == kRunning) {
    return RunMode::Running;
  } else if (str == kSprinting) {
    return RunMode::Sprinting;
  } else {
    throw std::runtime_error("cannot parse RunMode from " +
                             std::string{ str });
  }
}
