#include "PacketParser.h"
#include "AnimationData.h"
#include "Exceptions.h"
#include "HitData.h"
#include "JsonUtils.h"
#include "MovementMessage.h"
#include "MovementMessageSerialization.h"
#include "MpActor.h"
#include <MsgType.h>
#include <simdjson.h>
#include <slikenet/BitStream.h>

namespace FormIdCasts {
uint32_t LongToNormal(uint64_t longFormId)
{
  return static_cast<uint32_t>(longFormId % 0x100000000);
}
}

namespace JsonPointers {
static const JsonPointer t("t"), idx("idx"), content("content"), data("data"),
  pos("pos"), rot("rot"), isInJumpState("isInJumpState"),
  isWeapDrawn("isWeapDrawn"), isBlocking("isBlocking"),
  worldOrCell("worldOrCell"), inv("inv"), caster("caster"), target("target"),
  snippetIdx("snippetIdx"), returnValue("returnValue"), baseId("baseId"),
  commandName("commandName"), args("args"), workbench("workbench"),
  resultObjectId("resultObjectId"), craftInputObjects("craftInputObjects"),
  remoteId("remoteId"), eventName("eventName"), health("health"),
  magicka("magicka"), stamina("stamina"), leftSpell("leftSpell"),
  rightSpell("rightSpell"), voiceSpell("voiceSpell"),
  instantSpell("instantSpell");
}

struct PacketParser::Impl
{
  simdjson::dom::parser simdjsonParser;
};

PacketParser::PacketParser()
{
  pImpl.reset(new Impl);
}

void PacketParser::TransformPacketIntoAction(Networking::UserId userId,
                                             Networking::PacketData data,
                                             size_t length,
                                             ActionListener& actionListener)
{
  if (!length) {
    throw std::runtime_error("Zero-length message packets are not allowed");
  }

  ActionListener::RawMessageData rawMsgData{
    data,
    length,
    /*parsed (json)*/ {},
    userId,
  };

  if (length > 1 && data[1] == MovementMessage::kHeaderByte) {
    MovementMessage movData;
    // BitStream requires non-const ref even though it doesn't modify it
    SLNet::BitStream stream(const_cast<unsigned char*>(data) + 2, length - 2,
                            /*copyData*/ false);
    serialization::ReadFromBitStream(stream, movData);

    actionListener.OnUpdateMovement(
      rawMsgData, movData.idx,
      { movData.pos[0], movData.pos[1], movData.pos[2] },
      { movData.rot[0], movData.rot[1], movData.rot[2] },
      movData.isInJumpState, movData.isWeapDrawn, movData.isBlocking,
      movData.worldOrCell);
    return;
  }

  rawMsgData.parsed =
    pImpl->simdjsonParser.parse(data + 1, length - 1).value();

  const auto& jMessage = rawMsgData.parsed;

  using TypeInt = std::underlying_type<MsgType>::type;
  auto type = MsgType::Invalid;
  Read(jMessage, JsonPointers::t, reinterpret_cast<TypeInt*>(&type));

  switch (type) {
    case MsgType::Invalid:
      break;
    case MsgType::CustomPacket: {
      simdjson::dom::element content;
      Read(jMessage, JsonPointers::content, &content);
      actionListener.OnCustomPacket(rawMsgData, content);
    } break;
    case MsgType::UpdateMovement: {
      uint32_t idx;
      ReadEx(jMessage, JsonPointers::idx, &idx);

      simdjson::dom::element data_;
      Read(jMessage, JsonPointers::data, &data_);

      simdjson::dom::element jPos;
      Read(data_, JsonPointers::pos, &jPos);
      float pos[3];
      for (int i = 0; i < 3; ++i)
        ReadEx(jPos, i, &pos[i]);

      simdjson::dom::element jRot;
      Read(data_, JsonPointers::rot, &jRot);
      float rot[3];
      for (int i = 0; i < 3; ++i)
        ReadEx(jRot, i, &rot[i]);

      bool isInJumpState = false;
      Read(data_, JsonPointers::isInJumpState, &isInJumpState);

      bool isWeapDrawn = false;
      Read(data_, JsonPointers::isWeapDrawn, &isWeapDrawn);

      bool isBlocking = false;
      Read(data_, JsonPointers::isBlocking, &isBlocking);

      uint32_t worldOrCell = 0;
      ReadEx(data_, JsonPointers::worldOrCell, &worldOrCell);

      actionListener.OnUpdateMovement(
        rawMsgData, idx, { pos[0], pos[1], pos[2] },
        { rot[0], rot[1], rot[2] }, isInJumpState, isWeapDrawn, isBlocking,
        worldOrCell);

    } break;
    case MsgType::UpdateAnimation: {
      uint32_t idx;
      ReadEx(jMessage, JsonPointers::idx, &idx);
      simdjson::dom::element jData;
      ReadEx(jMessage, JsonPointers::data, &jData);
      actionListener.OnUpdateAnimation(rawMsgData, idx,
                                       AnimationData::FromJson(jData));
    } break;
    case MsgType::UpdateAppearance: {
      uint32_t idx;
      ReadEx(jMessage, JsonPointers::idx, &idx);
      simdjson::dom::element jData;
      Read(jMessage, JsonPointers::data, &jData);

      actionListener.OnUpdateAppearance(rawMsgData, idx,
                                        Appearance::FromJson(jData));
    } break;
    case MsgType::UpdateEquipment: {
      uint32_t idx;
      ReadEx(jMessage, JsonPointers::idx, &idx);
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      simdjson::dom::element inv;
      ReadEx(data_, JsonPointers::inv, &inv);

      uint32_t leftSpell = 0;

      if (data_.at_pointer(JsonPointers::leftSpell.GetData()).error() ==
          simdjson::error_code::SUCCESS) {
        ReadEx(data_, JsonPointers::leftSpell, &leftSpell);
      }

      uint32_t rightSpell = 0;

      if (data_.at_pointer(JsonPointers::rightSpell.GetData()).error() ==
          simdjson::error_code::SUCCESS) {
        ReadEx(data_, JsonPointers::rightSpell, &rightSpell);
      }

      uint32_t voiceSpell = 0;

      if (data_.at_pointer(JsonPointers::voiceSpell.GetData()).error() ==
          simdjson::error_code::SUCCESS) {
        ReadEx(data_, JsonPointers::voiceSpell, &voiceSpell);
      }

      uint32_t instantSpell = 0;

      if (data_.at_pointer(JsonPointers::instantSpell.GetData()).error() ==
          simdjson::error_code::SUCCESS) {
        ReadEx(data_, JsonPointers::instantSpell, &instantSpell);
      }

      actionListener.OnUpdateEquipment(rawMsgData, idx, data_,
                                       Inventory::FromJson(inv), leftSpell,
                                       rightSpell, voiceSpell, instantSpell);
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
      if (type == MsgType::PutItem) {
        e.extra.worn = Inventory::Worn::None;
        actionListener.OnPutItem(rawMsgData, target, e);
      } else {
        actionListener.OnTakeItem(rawMsgData, target, e);
      }
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
    case MsgType::ChangeValues: {
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      ActorValues actorValues;
      ReadEx(data_, JsonPointers::health, &actorValues.healthPercentage);
      ReadEx(data_, JsonPointers::magicka, &actorValues.magickaPercentage);
      ReadEx(data_, JsonPointers::stamina, &actorValues.staminaPercentage);
      actionListener.OnChangeValues(rawMsgData, actorValues);
      break;
    }
    case MsgType::OnHit: {
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      actionListener.OnHit(rawMsgData, HitData::FromJson(data_));
      break;
    }
    case MsgType::DropItem: {
      uint64_t baseId;
      ReadEx(jMessage, JsonPointers::baseId, &baseId);
      auto entry = Inventory::Entry::FromJson(jMessage);
      actionListener.OnDropItem(rawMsgData, FormIdCasts::LongToNormal(baseId),
                                entry);
      break;
    }
    default:
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      actionListener.OnUnknown(rawMsgData, data_);
      break;
  }
}
