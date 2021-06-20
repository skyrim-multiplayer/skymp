#include "PacketParser.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include "MpActor.h"
#include <MsgType.h>
#include <simdjson.h>

#include "Structures.h"

namespace FormIdCasts {
uint32_t LongToNormal(uint64_t longFormId)
{
  if (longFormId < 0x100000000)
    return static_cast<uint32_t>(longFormId);
  return static_cast<uint32_t>(longFormId % 0x100000000);
}
}

namespace JsonPointers {
static const JsonPointer t("t"), idx("idx"), content("content"), data("data"),
  pos("pos"), rot("rot"), isInJumpState("isInJumpState"),
  isWeapDrawn("isWeapDrawn"), worldOrCell("worldOrCell"), inv("inv"),
  caster("caster"), target("target"), snippetIdx("snippetIdx"),
  returnValue("returnValue"), baseId("baseId"), commandName("commandName"),
  args("args"), workbench("workbench"), resultObjectId("resultObjectId"),
  craftInputObjects("craftInputObjects"), remoteId("remoteId"),
  eventName("eventName");
}

struct PacketParser::Impl
{
  simdjson::dom::parser parser;
};

PacketParser::PacketParser()
{
  pImpl.reset(new Impl);
}

void PacketParser::TransformPacketIntoAction(Networking::UserId userId,
                                             Networking::PacketData data,
                                             size_t length,
                                             IActionListener& actionListener)
{
  if (!length)
    throw std::runtime_error("Zero-length message packets are not allowed");

  auto jMessage = pImpl->parser.parse(data + 1, length - 1).value();

  using TypeInt = std::underlying_type<MsgType>::type;
  auto type = MsgType::Invalid;
  Read(jMessage, JsonPointers::t, reinterpret_cast<TypeInt*>(&type));

  IActionListener::RawMessageData rawMsgData{ data, length, jMessage, userId };

  switch (type) {
    case MsgType::Invalid:
      break;
    case MsgType::CustomPacket: {
      simdjson::dom::element content;
      Read(jMessage, JsonPointers::content, &content);
      actionListener.OnCustomPacket(rawMsgData, content);
    } break;
    case MsgType::UpdateAnimation: {
      uint32_t idx;
      ReadEx(jMessage, JsonPointers::idx, &idx);
      actionListener.OnUpdateAnimation(rawMsgData, idx);
    } break;
    case MsgType::UpdateLook: {
      uint32_t idx;
      ReadEx(jMessage, JsonPointers::idx, &idx);
      simdjson::dom::element jData;
      Read(jMessage, JsonPointers::data, &jData);

      actionListener.OnUpdateLook(rawMsgData, idx, Look::FromJson(jData));
    } break;
    case MsgType::UpdateEquipment: {
      uint32_t idx;
      ReadEx(jMessage, JsonPointers::idx, &idx);
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      simdjson::dom::element inv;
      ReadEx(data_, JsonPointers::inv, &inv);

      actionListener.OnUpdateEquipment(rawMsgData, idx, data_,
                                       Inventory::FromJson(inv));
    } break;
    case MsgType::Activate: {
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      uint64_t caster, target;
      ReadEx(data_, JsonPointers::caster, &caster);
      ReadEx(data_, JsonPointers::target, &target);
      actionListener.OnActivate(rawMsgData, FormIdCasts::LongToNormal(caster),
                                FormIdCasts::LongToNormal(target));
    } break;
    case MsgType::UpdateProperty:
      break;
    case MsgType::PutItem:
    case MsgType::TakeItem: {
      uint32_t target;
      ReadEx(jMessage, JsonPointers::target, &target);
      auto e = Inventory::Entry::FromJson(jMessage);
      if (type == MsgType::PutItem)
        actionListener.OnPutItem(rawMsgData, target, e);
      else
        actionListener.OnTakeItem(rawMsgData, target, e);
    } break;
    case MsgType::FinishSpSnippet: {
      uint32_t snippetIdx;
      ReadEx(jMessage, JsonPointers::snippetIdx, &snippetIdx);

      simdjson::dom::element returnValue;
      ReadEx(jMessage, JsonPointers::returnValue, &returnValue);

      actionListener.OnFinishSpSnippet(rawMsgData, snippetIdx, returnValue);

      break;
    }
    case MsgType::OnEquip: {
      uint32_t baseId;
      ReadEx(jMessage, JsonPointers::baseId, &baseId);
      actionListener.OnEquip(rawMsgData, baseId);
      break;
    }
    case MsgType::ConsoleCommand: {
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      const char* commandName;
      ReadEx(data_, JsonPointers::commandName, &commandName);
      simdjson::dom::element args;
      ReadEx(data_, JsonPointers::args, &args);

      auto arr = args.get_array().value();

      std::vector<ConsoleCommands::Argument> consoleArgs;
      consoleArgs.resize(arr.size());
      for (size_t i = 0; i < arr.size(); ++i) {
        simdjson::dom::element el;
        ReadEx(args, i, &el);

        std::string s = simdjson::minify(el);
        if (!s.empty() && s[0] == '"') {
          const char* s;
          ReadEx(args, i, &s);
          consoleArgs[i] = std::string(s);
        } else {
          int64_t ingeter;
          ReadEx(args, i, &ingeter);
          consoleArgs[i] = ingeter;
        }
      }
      actionListener.OnConsoleCommand(rawMsgData, commandName, consoleArgs);
      break;
    }
    case MsgType::CraftItem: {
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      uint32_t workbench;
      ReadEx(data_, JsonPointers::workbench, &workbench);
      uint32_t resultObjectId;
      ReadEx(data_, JsonPointers::resultObjectId, &resultObjectId);
      simdjson::dom::element craftInputObjects;
      ReadEx(data_, JsonPointers::craftInputObjects, &craftInputObjects);
      actionListener.OnCraftItem(rawMsgData,
                                 Inventory::FromJson(craftInputObjects),
                                 workbench, resultObjectId);
      break;
    }
    case MsgType::Host: {
      uint64_t remoteId;
      ReadEx(jMessage, JsonPointers::remoteId, &remoteId);
      actionListener.OnHostAttempt(rawMsgData,
                                   FormIdCasts::LongToNormal(remoteId));
      break;
    }
    case MsgType::CustomEvent: {
      simdjson::dom::element args;
      ReadEx(jMessage, JsonPointers::args, &args);
      const char* eventName;
      ReadEx(jMessage, JsonPointers::eventName, &eventName);
      actionListener.OnCustomEvent(rawMsgData, eventName, args);
      break;
    }
    default:
      throw PublicError("Unknown MsgType: " + std::to_string((TypeInt)type));
  }
}
void PacketParser::TransformDataPacketInfoAction(
  Networking::UserId userId, Networking::PacketData packetData,
  size_t packetLength, IActionListener& actionListener)
{
  if (!packetLength)
    throw std::runtime_error("Zero-length message packets are not allowed");

  auto type = static_cast<BinaryMsgType>(packetData[0]);
  IActionListener::RawMessageBinaryData rawMsgData{
    packetData, packetLength, userId };
  auto newData = &packetData[1];

  switch (type) {
    case BinaryMsgType::UpdateMovement: {
      if (packetLength - 1 != Structures::MovementSize)
        throw std::runtime_error("Packet length is invalid");
      auto movement = static_cast<Structures::Movement*>((void*)newData);

      actionListener.OnUpdateMovement(
        rawMsgData, userId,
        { (float)movement->x, (float)movement->y, (float)movement->z },
        { 0, 0, movement->angleZ / 65535.f * 360.f },
        movement->movementFlags &
          static_cast<int>(Structures::MovementFlags::IsInJumpState),
        movement->movementFlags &
          static_cast<int>(Structures::MovementFlags::IsWeapDrawn),
        movement->worldOrCell);
    } break;
    default:
      break;
  }
}
