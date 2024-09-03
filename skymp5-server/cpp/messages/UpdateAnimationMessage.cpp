#include "UpdateAnimationMessage.h"
#include "SerializationUtil/BitStreamUtil.h"
#include "papyrus-vm/Utils.h" // stricmp
#include <nlohmann/json.hpp>

static const char* const kAnimEventNameDictionary[] = {
  "jumpStandingStart",
  "jumpDirectionalStart",
  "JumpFall",
  "JumpLandDirectional",
  "JumpLand",
  "attackStart",
  "attackStartLeftHand",
  "AttackStartH2HRight",
  "AttackStartH2HLeft",
  "bowAttackStart",
  "attackRelease",
  "crossbowAttackStart",
  "SneakSprintStartRoll",
  "attackStartDualWield",
  "attackPowerStartInPlace",
  "attackPowerStartBackward",
  "attackPowerStartLeft",
  "attackPowerStartRight",
  "attackPowerStartDualWield",
  "attackPowerStartForward",
  "attackPowerStart_2HWSprint",
  "attackStartSprint",
  "attackPowerStart_2HMSprint",
  "sprintStart",
  "sprintStop",
  "blockStart",
  "blockStop",
  "sneakStart",
  "sneakStop"
};

void UpdateAnimationMessage::WriteBinary(SLNet::BitStream& stream) const
{
  SerializationUtil::WriteToBitStream(stream, idx);
  SerializationUtil::WriteToBitStream(stream, numChanges);

  auto it =
    std::find_if(std::begin(kAnimEventNameDictionary),
                 std::end(kAnimEventNameDictionary), [&](const char* name) {
                   return Utils::stricmp(name, animEventName.c_str()) == 0;
                 });

  if (it == std::end(kAnimEventNameDictionary)) {
    SerializationUtil::WriteToBitStream(stream, animEventName);
  } else {
    SerializationUtil::WriteToBitStream(
      stream, static_cast<uint8_t>(it - std::begin(kAnimEventNameDictionary)));
  }
}

void UpdateAnimationMessage::ReadBinary(SLNet::BitStream& stream)
{
  SerializationUtil::ReadFromBitStream(stream, idx);
  SerializationUtil::ReadFromBitStream(stream, numChanges);

  if (stream.GetNumberOfUnreadBits() == 8) {
    uint8_t animEventNameIdx;
    SerializationUtil::ReadFromBitStream(stream, animEventNameIdx);
    constexpr auto size =
      sizeof(kAnimEventNameDictionary) / sizeof(kAnimEventNameDictionary[0]);
    if (animEventNameIdx >= size) {
      animEventName = "";
    } else {
      animEventName = kAnimEventNameDictionary[animEventNameIdx];
    }
  } else {
    SerializationUtil::ReadFromBitStream(stream, animEventName);
  }
}

void UpdateAnimationMessage::WriteJson(nlohmann::json& json) const
{
  auto result = nlohmann::json{
    { "t", MsgType::UpdateAnimation },
    { "idx", idx },
    {
      "data",
      {
        { "animEventName", animEventName },
        { "numChanges", numChanges },
      },
    },
  };
  json = std::move(result);
}

void UpdateAnimationMessage::ReadJson(const nlohmann::json& json)
{
  UpdateAnimationMessage result;
  result.idx = json.at("idx").get<uint32_t>();

  const auto& data = json.at("data");
  result.numChanges = data.at("numChanges");
  result.animEventName = data.at("animEventName");

  *this = result;
}
