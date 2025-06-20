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

  RawMessageData rawMsgData{
    data,
    length,
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
      case MsgType::Invalid: {
        return;
      }
      case MsgType::Activate: {
        auto message =
          reinterpret_cast<ActivateMessage*>(result->message.get());
        actionListener.OnActivate(
          rawMsgData, FormIdCasts::LongToNormal(message->data.caster),
          FormIdCasts::LongToNormal(message->data.target),
          message->data.isSecondActivation);
        return;
      }
      case MsgType::ConsoleCommand: {
        auto message =
          reinterpret_cast<ConsoleCommandMessage*>(result->message.get());

        std::vector<ConsoleCommands::Argument> consoleArgs;
        consoleArgs.resize(message->data.args.size());
        for (size_t i = 0; i < message->data.args.size(); i++) {
          consoleArgs[i] = ConsoleCommands::Argument(message->data.args[i]);
        }

        actionListener.OnConsoleCommand(rawMsgData, message->data.commandName,
                                        consoleArgs);
        return;
      }
      case MsgType::CraftItem: {
        auto message =
          reinterpret_cast<CraftItemMessage*>(result->message.get());
        actionListener.OnCraftItem(rawMsgData, message->data.craftInputObjects,
                                   message->data.workbench,
                                   message->data.resultObjectId);
        return;
      }
      case MsgType::CustomEvent: {
        auto message =
          reinterpret_cast<CustomEventMessage*>(result->message.get());
        actionListener.OnCustomEvent(rawMsgData, message->eventName.data(),
                                     message->argsJsonDumps);
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
      case MsgType::OnHit: {
        auto message = reinterpret_cast<HitMessage*>(result->message.get());
        actionListener.OnHit(rawMsgData, message->data);
        return;
      }
      case MsgType::Host: {
        auto message = reinterpret_cast<HostMessage*>(result->message.get());
        actionListener.OnHostAttempt(
          rawMsgData, FormIdCasts::LongToNormal(message->remoteId));
        return;
      }
      case MsgType::UpdateMovement: {
        auto message =
          reinterpret_cast<UpdateMovementMessage*>(result->message.get());
        actionListener.OnUpdateMovement(rawMsgData, *message);
        return;
      }
      case MsgType::UpdateAnimation: {
        auto message =
          reinterpret_cast<UpdateAnimationMessage*>(result->message.get());
        actionListener.OnUpdateAnimation(rawMsgData, *message);
        return;
      }
      case MsgType::UpdateEquipment: {
        auto message =
          reinterpret_cast<UpdateEquipmentMessage*>(result->message.get());
        actionListener.OnUpdateEquipment(rawMsgData, *message);
        return;
      }
      case MsgType::ChangeValues: {
        auto message =
          reinterpret_cast<ChangeValuesMessage*>(result->message.get());
        actionListener.OnChangeValues(rawMsgData, *message);
        return;
      }
      case MsgType::CustomPacket: {
        auto message =
          reinterpret_cast<CustomPacketMessage*>(result->message.get());
        auto parsedContent = pImpl->simdjsonParser
                               .parse(message->contentJsonDump.data(),
                                      message->contentJsonDump.size())
                               .value();
        actionListener.OnCustomPacket(rawMsgData, parsedContent);
        return;
      }
      case MsgType::UpdateAppearance: {
        auto message =
          reinterpret_cast<UpdateAppearanceMessage*>(result->message.get());
        actionListener.OnUpdateAppearance(rawMsgData, *message);
        return;
      }
      case MsgType::UpdateProperty: {
        return;
      }
      case MsgType::PutItem: {
        auto message =
          reinterpret_cast<PutItemMessage*>(result->message.get());
        const auto& extra = static_cast<Inventory::ExtraData&>(*message);
        const uint32_t baseId = message->baseId;
        const uint32_t count = message->count;
        const uint32_t target = message->target;

        Inventory::Entry entry;
        entry.baseId = baseId;
        entry.count = count;
        static_cast<Inventory::ExtraData&>(entry) = extra;

        entry.SetWorn(Inventory::Worn::None);

        actionListener.OnPutItem(rawMsgData, target, entry);
        return;
      }
      case MsgType::TakeItem: {
        auto message =
          reinterpret_cast<TakeItemMessage*>(result->message.get());
        const auto& extra = static_cast<Inventory::ExtraData&>(*message);
        const uint32_t baseId = message->baseId;
        const uint32_t count = message->count;
        const uint32_t target = message->target;

        Inventory::Entry entry;
        entry.baseId = baseId;
        entry.count = count;
        static_cast<Inventory::ExtraData&>(entry) = extra;

        actionListener.OnTakeItem(rawMsgData, target, entry);
        return;
      }
      case MsgType::FinishSpSnippet: {
        auto message =
          reinterpret_cast<FinishSpSnippetMessage*>(result->message.get());
        const std::optional<std::variant<bool, double, std::string>>&
          returnValue = message->returnValue;
        const uint32_t snippetIdx = static_cast<uint32_t>(message->snippetIdx);

        actionListener.OnFinishSpSnippet(rawMsgData, snippetIdx, returnValue);
        return;
      }
      case MsgType::OnEquip: {
        auto message =
          reinterpret_cast<OnEquipMessage*>(result->message.get());
        actionListener.OnEquip(rawMsgData, message->baseId);
        return;
      }
      case MsgType::SpellCast: {
        auto message =
          reinterpret_cast<SpellCastMessage*>(result->message.get());
        actionListener.OnSpellCast(rawMsgData, message->data);
        return;
      }
      case MsgType::UpdateAnimVariables: {
        actionListener.OnUpdateAnimVariables(rawMsgData);
        return;
      }
      case MsgType::PlayerBowShot: {
        auto message =
          reinterpret_cast<PlayerBowShotMessage*>(result->message.get());
        actionListener.OnPlayerBowShot(rawMsgData, message->weaponId,
                                       message->ammoId, message->power,
                                       message->isSunGazing);
        break;
      }
      default: {
        spdlog::error("PacketParser.cpp doesn't implement MsgType {}",
                      static_cast<int64_t>(result->msgType));
        return;
      }
    }
  }
}
