#include "MovementMessageSerialization.h"

#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

#include "MsgType.h"
#include "SerializationUtil/BitStreamUtil.h"

namespace serialization {

void WriteToBitStream(SLNet::BitStream& stream, const MovementMessage& movData)
{
  using SerializationUtil::WriteToBitStream;

  WriteToBitStream(stream, movData.idx);
  WriteToBitStream(stream, movData.worldOrCell);
  WriteToBitStream(stream, movData.pos);
  WriteToBitStream(stream, movData.rot);
  WriteToBitStream(stream, movData.direction);
  WriteToBitStream(stream, movData.healthPercentage);
  WriteToBitStream(stream, movData.speed);

  WriteToBitStream(
    stream, static_cast<bool>(static_cast<uint8_t>(movData.runMode) & 2));
  WriteToBitStream(
    stream, static_cast<bool>(static_cast<uint8_t>(movData.runMode) & 1));

  WriteToBitStream(stream, movData.isInJumpState);
  WriteToBitStream(stream, movData.isSneaking);
  WriteToBitStream(stream, movData.isBlocking);
  WriteToBitStream(stream, movData.isWeapDrawn);
  WriteToBitStream(stream, movData.isDead);
  WriteToBitStream(stream, movData.lookAt);
}

void ReadFromBitStream(SLNet::BitStream& stream, MovementMessage& movData)
{
  using SerializationUtil::ReadFromBitStream;

  ReadFromBitStream(stream, movData.idx);
  ReadFromBitStream(stream, movData.worldOrCell);
  ReadFromBitStream(stream, movData.pos);
  ReadFromBitStream(stream, movData.rot);
  ReadFromBitStream(stream, movData.direction);
  ReadFromBitStream(stream, movData.healthPercentage);
  ReadFromBitStream(stream, movData.speed);

  uint8_t runMode = 0;
  runMode |= static_cast<uint8_t>(ReadFromBitStream<bool>(stream));
  runMode <<= 1;
  runMode |= static_cast<uint8_t>(ReadFromBitStream<bool>(stream));
  movData.runMode = static_cast<RunMode>(runMode);

  ReadFromBitStream(stream, movData.isInJumpState);
  ReadFromBitStream(stream, movData.isSneaking);
  ReadFromBitStream(stream, movData.isBlocking);
  ReadFromBitStream(stream, movData.isWeapDrawn);
  ReadFromBitStream(stream, movData.isDead);
  ReadFromBitStream(stream, movData.lookAt);
}

MovementMessage MovementMessageFromJson(const nlohmann::json& json)
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
  result.runMode =
    RunModeFromString(data.at("runMode").get<std::string_view>());
  result.isInJumpState = data.at("isInJumpState").get<bool>();
  result.isSneaking = data.at("isSneaking").get<bool>();
  result.isBlocking = data.at("isBlocking").get<bool>();
  result.isWeapDrawn = data.at("isWeapDrawn").get<bool>();
  result.isDead = data.at("isDead").get<bool>();
  const auto lookAtIt = data.find("lookAt");
  if (lookAtIt != data.end()) {
    result.lookAt = lookAtIt->get<std::array<float, 3>>();
  }
  return result;
}

nlohmann::json MovementMessageToJson(const MovementMessage& movData)
{
  auto result = nlohmann::json{
    { "t", MsgType::UpdateMovement },
    { "idx", movData.idx },
    {
      "data",
      {
        { "worldOrCell", movData.worldOrCell },
        { "pos", movData.pos },
        { "rot", movData.rot },
        { "runMode", ToString(movData.runMode) },
        { "direction", movData.direction },
        { "healthPercentage", movData.healthPercentage },
        { "speed", movData.speed },
        { "isInJumpState", movData.isInJumpState },
        { "isSneaking", movData.isSneaking },
        { "isBlocking", movData.isBlocking },
        { "isWeapDrawn", movData.isWeapDrawn },
        { "isDead", movData.isDead },
      },
    },
  };
  if (movData.lookAt) {
    result["data"]["lookAt"] = *movData.lookAt;
  }
  return result;
}

}
