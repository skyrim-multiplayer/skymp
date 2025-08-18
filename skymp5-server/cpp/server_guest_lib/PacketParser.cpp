#include "PacketParser.h"
#include "AnimationData.h"
#include "Exceptions.h"
#include "HitData.h"
#include "JsonUtils.h"
#include "MessageEvent.h"
#include "MessageSerializerFactory.h"
#include "Messages.h"
#include "MpActor.h"
#include "MsgType.h"
#include "PartOne.h"
#include "SpellCastData.h"
#include <simdjson.h>
#include <slikenet/BitStream.h>

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
                                             PartOne& partOne)
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
        partOne.onActivateMessage(MessageEvent<ActivateMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::ConsoleCommand: {
        auto message =
          reinterpret_cast<ConsoleCommandMessage*>(result->message.get());
        partOne.onConsoleCommandMessage(MessageEvent<ConsoleCommandMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::CraftItem: {
        auto message =
          reinterpret_cast<CraftItemMessage*>(result->message.get());
        partOne.onCraftItemMessage(MessageEvent<CraftItemMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::CustomEvent: {
        auto message =
          reinterpret_cast<CustomEventMessage*>(result->message.get());
        partOne.onCustomEventMessage(MessageEvent<CustomEventMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::DropItem: {
        auto message =
          reinterpret_cast<DropItemMessage*>(result->message.get());
        partOne.onDropItemMessage(MessageEvent<DropItemMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::OnHit: {
        auto message = reinterpret_cast<HitMessage*>(result->message.get());
        partOne.onHitMessage(MessageEvent<HitMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::Host: {
        auto message = reinterpret_cast<HostMessage*>(result->message.get());
        partOne.onHostMessage(MessageEvent<HostMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::UpdateMovement: {
        auto message =
          reinterpret_cast<UpdateMovementMessage*>(result->message.get());
        partOne.onUpdateMovementMessage(MessageEvent<UpdateMovementMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::UpdateAnimation: {
        auto message =
          reinterpret_cast<UpdateAnimationMessage*>(result->message.get());
        partOne.onUpdateAnimationMessage(MessageEvent<UpdateAnimationMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::UpdateEquipment: {
        auto message =
          reinterpret_cast<UpdateEquipmentMessage*>(result->message.get());
        partOne.onUpdateEquipmentMessage(MessageEvent<UpdateEquipmentMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::ChangeValues: {
        auto message =
          reinterpret_cast<ChangeValuesMessage*>(result->message.get());
        partOne.onChangeValuesMessage(MessageEvent<ChangeValuesMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::CustomPacket: {
        auto message =
          reinterpret_cast<CustomPacketMessage*>(result->message.get());
        partOne.onCustomPacketMessage(MessageEvent<CustomPacketMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::UpdateAppearance: {
        auto message =
          reinterpret_cast<UpdateAppearanceMessage*>(result->message.get());
        partOne.onUpdateAppearanceMessage(MessageEvent<UpdateAppearanceMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::UpdateProperty: {
        return;
      }
      case MsgType::PutItem: {
        auto message =
          reinterpret_cast<PutItemMessage*>(result->message.get());
        partOne.onPutItemMessage(MessageEvent<PutItemMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::TakeItem: {
        auto message =
          reinterpret_cast<TakeItemMessage*>(result->message.get());
        partOne.onTakeItemMessage(MessageEvent<TakeItemMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::FinishSpSnippet: {
        auto message =
          reinterpret_cast<FinishSpSnippetMessage*>(result->message.get());
        partOne.onFinishSpSnippetMessage(MessageEvent<FinishSpSnippetMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::OnEquip: {
        auto message =
          reinterpret_cast<OnEquipMessage*>(result->message.get());
        partOne.onOnEquipMessage(MessageEvent<OnEquipMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::SpellCast: {
        auto message =
          reinterpret_cast<SpellCastMessage*>(result->message.get());
        partOne.onSpellCastMessage(MessageEvent<SpellCastMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::UpdateAnimVariables: {
        auto message =
          reinterpret_cast<UpdateAnimVariablesMessage*>(result->message.get());
        partOne.onUpdateAnimVariablesMessage(MessageEvent<UpdateAnimVariablesMessage>{rawMsgData, *message});
        return;
      }
      case MsgType::PlayerBowShot: {
        auto message =
          reinterpret_cast<PlayerBowShotMessage*>(result->message.get());
        partOne.onPlayerBowShotMessage(MessageEvent<PlayerBowShotMessage>{rawMsgData, *message});
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
