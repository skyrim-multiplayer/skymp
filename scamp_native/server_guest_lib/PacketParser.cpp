#include "PacketParser.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include "MpActor.h"
#include <MsgType.h>
#include <simdjson.h>

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
  Read(jMessage, "t", reinterpret_cast<TypeInt*>(&type));

  IActionListener::RawMessageData rawMsgData{ data, length, jMessage, userId };

  switch (type) {
    case MsgType::Invalid:
      break;
    case MsgType::CustomPacket: {
      simdjson::dom::element content;
      Read(jMessage, "content", &content);
      actionListener.OnCustomPacket(rawMsgData, content);
    } break;
    case MsgType::UpdateMovement: {
      uint32_t idx;
      ReadEx(jMessage, "idx", &idx);

      simdjson::dom::element data_;
      Read(jMessage, "data", &data_);

      simdjson::dom::element jPos;
      Read(data_, "pos", &jPos);
      float pos[3];
      for (int i = 0; i < 3; ++i)
        ReadEx(jPos, i, &pos[i]);

      simdjson::dom::element jRot;
      Read(data_, "rot", &jRot);
      float rot[3];
      for (int i = 0; i < 3; ++i)
        ReadEx(jRot, i, &rot[i]);

      bool isInJumpState = false;
      Read(data_, "isInJumpState", &isInJumpState);

      actionListener.OnUpdateMovement(
        rawMsgData, idx, { pos[0], pos[1], pos[2] },
        { rot[0], rot[1], rot[2] }, isInJumpState);

    } break;
    case MsgType::UpdateAnimation: {
      uint32_t idx;
      ReadEx(jMessage, "idx", &idx);
      actionListener.OnUpdateAnimation(rawMsgData, idx);
    } break;
    case MsgType::UpdateLook: {
      uint32_t idx;
      ReadEx(jMessage, "idx", &idx);
      simdjson::dom::element jData;
      Read(jMessage, "data", &jData);

      actionListener.OnUpdateLook(rawMsgData, idx, Look::FromJson(jData));
    } break;
    case MsgType::UpdateEquipment: {
      uint32_t idx;
      ReadEx(jMessage, "idx", &idx);
      simdjson::dom::element data_;
      ReadEx(jMessage, "data", &data_);
      simdjson::dom::element inv;
      ReadEx(data_, "inv", &inv);

      actionListener.OnUpdateEquipment(rawMsgData, idx, data_,
                                       Inventory::FromJson(inv));
    } break;
    case MsgType::Activate: {
      simdjson::dom::element data_;
      ReadEx(jMessage, "data", &data_);
      uint32_t caster, target;
      ReadEx(data_, "caster", &caster);
      ReadEx(data_, "target", &target);
      actionListener.OnActivate(rawMsgData, caster, target);
    } break;
    case MsgType::UpdateProperty:
      break;
    case MsgType::PutItem:
    case MsgType::TakeItem: {
      uint32_t target;
      ReadEx(jMessage, "target", &target);
      auto e = Inventory::Entry::FromJson(jMessage);
      if (type == MsgType::PutItem)
        actionListener.OnPutItem(rawMsgData, target, e);
      else
        actionListener.OnTakeItem(rawMsgData, target, e);
    } break;
    case MsgType::FinishSpSnippet: {
      uint32_t snippetIdx;
      ReadEx(jMessage, "snippetIdx", &snippetIdx);

      simdjson::dom::element returnValue;
      ReadEx(jMessage, "returnValue", &returnValue);

      actionListener.OnFinishSpSnippet(rawMsgData, snippetIdx, returnValue);

      break;
    }
    case MsgType::OnEquip: {
      uint32_t baseId;
      ReadEx(jMessage, "baseId", &baseId);
      actionListener.OnEquip(rawMsgData, baseId);
      break;
    }
    default:
      throw PublicError("Unknown MsgType: " + std::to_string((TypeInt)type));
  }
}