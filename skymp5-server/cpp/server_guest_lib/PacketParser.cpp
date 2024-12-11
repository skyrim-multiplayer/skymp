#include "PacketParser.h"
#include "AnimationData.h"
#include "Exceptions.h"
#include "HitData.h"
#include "JsonUtils.h"
#include "MessageSerializerFactory.h"
#include "Messages.h"
#include "MpActor.h"
#include "MsgType.h"
#include "SpellCastData.h"
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
  instantSpell("instantSpell"), weaponId("weaponId"), ammoId("ammoId"),
  power("power"), isSunGazing("isSunGazing"),
  isSecondActivation("isSecondActivation");
}

struct PacketParser::Impl
{
  simdjson::dom::parser simdjsonParser;
  std::shared_ptr<MessageSerializer> serializer;
  std::once_flag jsonWarning;
};

PacketParser::PacketParser()
{
  pImpl.reset(new Impl);
  pImpl->serializer = MessageSerializerFactory::CreateMessageSerializer();
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

  auto result = pImpl->serializer->Deserialize(data, length);
  if (result != std::nullopt) {
    if (result->format == DeserializeInputFormat::Json) {
      std::call_once(pImpl->jsonWarning, [&] {
        spdlog::warn("PacketParser::TransformPacketIntoAction - 1-st time "
                     "encountered a JSON packet, userId={}, msgType={}",
                     userId, static_cast<int64_t>(result->msgType));
      });
    }
    switch (result->msgType) {
      case MsgType::Activate: {
        auto message =
          reinterpret_cast<ActivateMessage*>(result->message.get());
        actionListener.OnActivate(rawMsgData,
                                  FormIdCasts::LongToNormal(message->caster),
                                  FormIdCasts::LongToNormal(message->target),
                                  message->isSecondActivation);
        return;
      }
      case MsgType::ConsoleCommand: {
        auto message =
          reinterpret_cast<ConsoleCommandMessage*>(result->message.get());

        std::vector<ConsoleCommands::Argument> consoleArgs;
        consoleArgs.resize(message->args.size());
        for (size_t i = 0; i < message->args.size(); i++) {
          consoleArgs[i] = ConsoleCommands::Argument(message->args[i]);
        }

        actionListener.OnConsoleCommand(rawMsgData, message->commandName,
                                        consoleArgs);
        return;
      }
      case MsgType::CraftItem: {
        auto message =
          reinterpret_cast<CraftItemMessage*>(result->message.get());
        actionListener.OnCraftItem(rawMsgData, message->craftInputObjects,
                                   message->workbench,
                                   message->resultObjectId);
        return;
      }
      case MsgType::CustomEvent: {
        // TODO:
        return;
      }
      case MsgType::DropItem: {
        auto message =
          reinterpret_cast<DropItemMessage*>(result->message.get());

        Inventory::Entry entry;
        entry.baseId = FormIdCasts::LongToNormal(message->baseId);
        entry.count = message->count;

        actionListener.OnDropItem(
          rawMsgData, FormIdCasts::LongToNormal(message->baseId), entry);
        return;
      }
      case MsgType::FinishSpSnippet: {
        // TODO:
        return;
      }
      case MsgType::OnHit: {
        // TODO:
        return;
      }
      case MsgType::Host: {
        // TODO:
        return;
      }
      case MsgType::UpdateMovement: {
        auto message =
          reinterpret_cast<UpdateMovementMessage*>(result->message.get());
        actionListener.OnUpdateMovement(
          rawMsgData, message->idx,
          { message->data.pos[0], message->data.pos[1], message->data.pos[2] },
          { message->data.rot[0], message->data.rot[1], message->data.rot[2] },
          message->data.isInJumpState, message->data.isWeapDrawn,
          message->data.isBlocking, message->data.worldOrCell,
          message->data.runMode);
        return;
      }
      case MsgType::UpdateAnimation: {
        auto message =
          reinterpret_cast<UpdateAnimationMessage*>(result->message.get());
        AnimationData animationData;
        animationData.animEventName = message->data.animEventName;
        animationData.numChanges = message->data.numChanges;
        actionListener.OnUpdateAnimation(rawMsgData, message->idx,
                                         animationData);
        return;
      }
      case MsgType::UpdateEquipment: {
        auto message =
          reinterpret_cast<UpdateEquipmentMessage*>(result->message.get());
        auto idx = message->idx;

        const Equipment& data = message->data;
        const Inventory& inv = data.inv;
        uint32_t leftSpell = data.leftSpell.value_or(0);
        uint32_t rightSpell = data.rightSpell.value_or(0);
        uint32_t voiceSpell = data.voiceSpell.value_or(0);
        uint32_t instantSpell = data.instantSpell.value_or(0);

        actionListener.OnUpdateEquipment(rawMsgData, idx, data, inv, leftSpell,
                                         rightSpell, voiceSpell, instantSpell);
        return;
      }
      case MsgType::ChangeValues: {
        auto message =
          reinterpret_cast<ChangeValuesMessage*>(result->message.get());
        ActorValues actorValues;
        actorValues.healthPercentage = message->data.health;
        actorValues.magickaPercentage = message->data.magicka;
        actorValues.staminaPercentage = message->data.stamina;
        actionListener.OnChangeValues(rawMsgData, actorValues);
        return;
      }
      default: {
        // likel a binary packet, can't just fall back to simdjson parsing
        spdlog::error("PacketParser.cpp doesn't implement MsgType {}",
                      static_cast<int64_t>(result->msgType));
        return;
      }
    }
  }

  rawMsgData.parsed =
    pImpl->simdjsonParser.parse(data + 1, length - 1).value();

  const auto& jMessage = rawMsgData.parsed;

  int64_t type = static_cast<int64_t>(MsgType::Invalid);
  Read(jMessage, JsonPointers::t, &type);

  switch (static_cast<MsgType>(type)) {
    case MsgType::Invalid:
      break;
    case MsgType::CustomPacket: {
      simdjson::dom::element content;
      Read(jMessage, JsonPointers::content, &content);
      actionListener.OnCustomPacket(rawMsgData, content);
    } break;
    case MsgType::UpdateAppearance: {
      uint32_t idx;
      ReadEx(jMessage, JsonPointers::idx, &idx);
      simdjson::dom::element jData;
      Read(jMessage, JsonPointers::data, &jData);

      actionListener.OnUpdateAppearance(rawMsgData, idx,
                                        Appearance::FromJson(jData));
    } break;
    case MsgType::Activate: {
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      uint64_t caster, target;
      ReadEx(data_, JsonPointers::caster, &caster);
      ReadEx(data_, JsonPointers::target, &target);
      bool isSecondActivation;
      ReadEx(data_, JsonPointers::isSecondActivation, &isSecondActivation);
      actionListener.OnActivate(rawMsgData, FormIdCasts::LongToNormal(caster),
                                FormIdCasts::LongToNormal(target),
                                isSecondActivation);
    } break;
    case MsgType::UpdateProperty:
      break;
    case MsgType::PutItem:
    case MsgType::TakeItem: {
      uint32_t target;
      ReadEx(jMessage, JsonPointers::target, &target);
      auto e = Inventory::Entry::FromJson(jMessage);
      if (static_cast<MsgType>(type) == MsgType::PutItem) {
        e.SetWorn(Inventory::Worn::None);
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
    case MsgType::OnHit: {
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      actionListener.OnHit(rawMsgData, HitData::FromJson(data_));
      break;
    }
    case MsgType::SpellCast: {
      simdjson::dom::element data_;
      ReadEx(jMessage, JsonPointers::data, &data_);
      actionListener.OnSpellCast(rawMsgData, SpellCastData::FromJson(data_));
      break;
    }
    case MsgType::UpdateAnimVariables: {
      actionListener.OnUpdateAnimVariables(rawMsgData);
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
    case MsgType::PlayerBowShot: {
      uint32_t weaponId;
      ReadEx(jMessage, JsonPointers::weaponId, &weaponId);
      uint32_t ammoId;
      ReadEx(jMessage, JsonPointers::ammoId, &ammoId);
      float power;
      ReadEx(jMessage, JsonPointers::power, &power);
      bool isSunGazing;
      ReadEx(jMessage, JsonPointers::isSunGazing, &isSunGazing);
      actionListener.OnPlayerBowShot(rawMsgData, weaponId, ammoId, power,
                                     isSunGazing);
      break;
    }
    default:
      actionListener.OnUnknown(rawMsgData);
      break;
  }
}
