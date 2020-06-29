#include "PartOne.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include "MsgType.h"
#include "ServerState.h"
#include "WorldState.h"
#include <array>
#include <cassert>
#include <optional>
#include <vector>

struct PartOne::Impl
{
  ServerState serverState;
  simdjson::dom::parser parser;
  std::vector<std::shared_ptr<Listener>> listeners;
};

PartOne::PartOne()
{
  pImpl.reset(new Impl);
}

PartOne::PartOne(std::shared_ptr<Listener> listener)
  : PartOne()
{
  AddListener(listener);
}

PartOne::~PartOne()
{
  // worldState may depend on serverState (actorsMap), we should reset it first
  worldState = {};
  pImpl->serverState = {};
}

void PartOne::AddListener(std::shared_ptr<Listener> listener)
{
  pImpl->listeners.push_back(listener);
}

bool PartOne::IsConnected(Networking::UserId userId) const
{
  return pImpl->serverState.IsConnected(userId);
}

void PartOne::CreateActor(uint32_t formId, const NiPoint3& pos, float angleZ,
                          uint32_t cellOrWorld,
                          Networking::ISendTarget* sendTarget)
{
  auto serverState = &pImpl->serverState;

  auto onSubscribe = [sendTarget, serverState](MpActor* emitter,
                                               MpActor* listener) {
    auto& emitterPos = emitter->GetPos();
    auto& emitterRot = emitter->GetAngle();

    bool isMe = emitter == listener;

    char data[1024] = { 0 };
    data[0] = Networking::MinPacketId;
    auto len = (size_t)snprintf(
      data + 1, std::size(data) - 1,
      R"({"type": "createActor", "formId": %u, "isMe": %s, "transform": {"pos": [%f,%f,%f],
    "rot": [%f,%f,%f], "worldOrCell": %u}})",
      emitter->GetFormId(), isMe ? "true" : "false", emitterPos.x,
      emitterPos.y, emitterPos.z, emitterRot.x, emitterRot.y, emitterRot.z,
      emitter->GetCellOrWorld());

    auto listenerUserId = serverState->UserByActor(listener);
    sendTarget->Send(listenerUserId,
                     reinterpret_cast<Networking::PacketData>(data), len + 1,
                     true);
  };

  auto onUnsubscribe = [sendTarget, serverState](MpActor* emitter,
                                                 MpActor* listener) {
    char data[1024] = { 0 };
    data[0] = Networking::MinPacketId;
    auto len = (size_t)snprintf(data + 1, std::size(data) - 1,
                                R"({"type": "destroyActor", "formId": %u})",
                                emitter->GetFormId());

    auto listenerUserId = serverState->UserByActor(listener);
    if (listenerUserId != Networking::InvalidUserId)
      sendTarget->Send(listenerUserId,
                       reinterpret_cast<Networking::PacketData>(data), len + 1,
                       true);
  };

  worldState.AddForm(
    std::unique_ptr<MpActor>(new MpActor(
      { pos, { 0, 0, angleZ }, cellOrWorld }, onSubscribe, onUnsubscribe)),
    formId);
}

void PartOne::SetUserActor(Networking::UserId userId, uint32_t actorFormId,
                           Networking::ISendTarget* sendTarget)
{
  if (!pImpl->serverState.userInfo[userId])
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");

  if (actorFormId > 0) {
    auto form = worldState.LookupFormById(actorFormId);
    if (!form) {
      std::stringstream ss;
      ss << "Form with id " << std::hex << actorFormId << " doesn't exist";
      throw std::runtime_error(ss.str());
    }

    auto actor = std::dynamic_pointer_cast<MpActor>(form);
    if (!actor) {
      std::stringstream ss;
      ss << "Form with id " << std::hex << actorFormId << " is not Actor";
      throw std::runtime_error(ss.str());
    }

    pImpl->serverState.actorsMap.insert({ userId, actor.get() });

    // Hacky way to force self-subscribing
    actor->SetPos(actor->GetPos());
  } else {
    pImpl->serverState.actorsMap.left.erase(userId);
  }
}

uint32_t PartOne::GetUserActor(Networking::UserId userId)
{
  auto& user = pImpl->serverState.userInfo[userId];
  if (!user)
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");

  auto actor = pImpl->serverState.ActorByUser(userId);
  if (!actor)
    return 0;
  return actor->GetFormId();
}

void PartOne::DestroyActor(uint32_t actorFormId)
{
  std::shared_ptr<MpActor> destroyedForm;
  worldState.DestroyForm<MpActor>(actorFormId, &destroyedForm);

  pImpl->serverState.actorsMap.right.erase(destroyedForm.get());
}

void PartOne::HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length)
{
  auto this_ = reinterpret_cast<PartOne*>(partOneInstance);

  switch (packetType) {
    case Networking::PacketType::ServerSideUserConnect:
      this_->pImpl->serverState.Connect(userId);
      for (auto& listener : this_->pImpl->listeners)
        listener->OnConnect(userId);
      return;
    case Networking::PacketType::ServerSideUserDisconnect:
      for (auto& listener : this_->pImpl->listeners)
        listener->OnDisconnect(userId);
      this_->pImpl->serverState.Disconnect(userId);
      return;
    case Networking::PacketType::Message:
      return this_->HandleMessagePacket(userId, data, length);
    default:
      throw std::runtime_error("Unexpected PacketType: " +
                               std::to_string((int)packetType));
  }
}

void PartOne::HandleMessagePacket(Networking::UserId userId,
                                  Networking::PacketData data, size_t length)
{
  if (!pImpl->serverState.IsConnected(userId))
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");
  auto& user = *pImpl->serverState.userInfo[userId];

  if (!length)
    throw std::runtime_error("Zero-length message packets are not allowed");
  auto jMessage = pImpl->parser.parse(data + 1, length - 1).value();

  using TypeInt = std::underlying_type<MsgType>::type;
  auto type = MsgType::Invalid;
  Read(jMessage, "t", reinterpret_cast<TypeInt*>(&type));

  switch (type) {
    case MsgType::CustomPacket: {
      simdjson::dom::element content;
      Read(jMessage, "content", &content);
      for (auto& listener : pImpl->listeners) {
        listener->OnCustomPacket(userId, content);
      }
      break;
    }
    case MsgType::UpdateMovement: {
      simdjson::dom::element data_;
      Read(jMessage, "data", &data_);
      int64_t formId = 0;
      Read(jMessage, "formId", &formId);
      if (MpActor* actor = pImpl->serverState.ActorByUser(userId)) {
        if (formId != actor->GetFormId()) {
          std::stringstream ss;
          ss << std::hex << "You aren't able to update actor with id "
             << formId << " (your actor's id is " << actor->GetFormId() << ')';
          throw PublicError(ss.str());
        }

        {
          simdjson::dom::element jPos;
          Read(data_, "pos", &jPos);
          double pos[3];
          for (int i = 0; i < 3; ++i)
            Read(jPos, i, &pos[i]);
          actor->SetPos({ (float)pos[0], (float)pos[1], (float)pos[2] });
        }
        {
          simdjson::dom::element jRot;
          Read(data_, "rot", &jRot);
          double rot[3];
          for (int i = 0; i < 3; ++i)
            Read(jRot, i, &rot[i]);
          actor->SetAngle({ (float)rot[0], (float)rot[1], (float)rot[2] });
        }

        // Send my movement to all
        for (auto listener : actor->GetListeners()) {
          auto targetuserId = pImpl->serverState.UserByActor(listener);
          if (targetuserId == Networking::InvalidUserId)
            return;
          pushedSendTarget->Send(targetuserId, data, length, false);
        }
      }
      break;
    }
    default:
      throw PublicError("Unknown MsgType: " + std::to_string((TypeInt)type));
  }
}