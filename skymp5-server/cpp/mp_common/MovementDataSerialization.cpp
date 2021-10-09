#include "MovementDataSerialization.h"

#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

#include "MsgType.h"

namespace {
void Write(const std::array<float, 3>& arr, SLNet::BitStream& stream)
{
  for (size_t i = 0; i < 3; ++i) {
    stream.Write(arr[i]);
  }
}

void ReadTo(std::array<float, 3>& arr, SLNet::BitStream& stream)
{
  for (size_t i = 0; i < 3; ++i) {
    stream.Read(arr[i]);
  }
}

template <class T>
T Read(SLNet::BitStream& stream)
{
  T value;
  stream.Read(value);
  return value;
}
}

void Write(const MovementData& movData, SLNet::BitStream& stream)
{
  stream.Write(movData.idx);
  stream.Write(movData.worldOrCell);
  Write(movData.pos, stream);
  Write(movData.rot, stream);
  stream.Write(movData.direction);
  stream.Write(movData.healthPercentage);

  stream.Write(static_cast<bool>(static_cast<uint8_t>(movData.runMode) & 2));
  stream.Write(static_cast<bool>(static_cast<uint8_t>(movData.runMode) & 1));

  stream.Write(movData.isInJumpState);
  stream.Write(movData.isSneaking);
  stream.Write(movData.isBlocking);
  stream.Write(movData.isWeapDrawn);
  if (movData.lookAt) {
    stream.Write(true);
    Write(*movData.lookAt, stream);
  } else {
    stream.Write(false);
  }
}

void ReadTo(MovementData& movData, SLNet::BitStream& stream)
{
  stream.Read(movData.idx);
  stream.Read(movData.worldOrCell);
  ReadTo(movData.pos, stream);
  ReadTo(movData.rot, stream);
  stream.Read(movData.direction);
  stream.Read(movData.healthPercentage);

  uint8_t runMode = 0;
  runMode |= static_cast<uint8_t>(Read<bool>(stream));
  runMode <<= 1;
  runMode |= static_cast<uint8_t>(Read<bool>(stream));
  movData.runMode = static_cast<RunMode>(runMode);

  stream.Read(movData.isInJumpState);
  stream.Read(movData.isSneaking);
  stream.Read(movData.isBlocking);
  stream.Read(movData.isWeapDrawn);
  if (Read<bool>(stream)) {
    ReadTo(movData.lookAt.emplace(), stream);
  }
}

MovementData MovementDataFromJson(const nlohmann::json& json)
{
  MovementData result;
  result.idx = json.at("idx").get<uint32_t>();
  result.worldOrCell = json.at("data").at("worldOrCell").get<uint32_t>();
  result.pos = json.at("data").at("pos").get<std::array<float, 3>>();
  result.rot = json.at("data").at("rot").get<std::array<float, 3>>();
  result.direction = json.at("data").at("direction").get<float>();
  result.healthPercentage = json.at("data").at("healthPercentage").get<float>();
  result.runMode =
    RunModeFromString(json.at("data").at("runMode").get<std::string_view>());
  result.isInJumpState = json.at("data").at("isInJumpState").get<bool>();
  result.isSneaking = json.at("data").at("isSneaking").get<bool>();
  result.isBlocking = json.at("data").at("isBlocking").get<bool>();
  result.isWeapDrawn = json.at("data").at("isWeapDrawn").get<bool>();
  const auto& data = json.at("data");
  const auto lookAtIt = data.find("lookAt");
  if (lookAtIt != data.end()) {
    result.lookAt = lookAtIt->get<std::array<float, 3>>();
  }
  return result;
}

nlohmann::json MovementDataToJson(const MovementData& movData)
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
        { "isInJumpState", movData.isInJumpState },
        { "isSneaking", movData.isSneaking },
        { "isBlocking", movData.isBlocking },
        { "isWeapDrawn", movData.isWeapDrawn },
      },
    },
  };
  if (movData.lookAt) {
    result["data"]["lookAt"] = *movData.lookAt;
  }
  return result;
}
