#include "PartOne.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include "MsgType.h"
#include "ServerState.h"
#include "WorldState.h"
#include <array>
#include <cassert>
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

    const char *lookPrefix = "", *look = "";
    auto& jLook = emitter->GetLookAsJson();
    if (!jLook.empty()) {
      lookPrefix = R"(, "look": )";
      look = jLook.data();
    }

    auto listenerUserId = serverState->UserByActor(listener);
    Networking::SendFormatted(
      sendTarget, listenerUserId,
      R"({"type": "createActor", "idx": %u, "isMe": %s, "transform": {"pos":
    [%f,%f,%f], "rot": [%f,%f,%f], "worldOrCell": %u}%s%s})",
      emitter->GetIdx(), isMe ? "true" : "false", emitterPos.x, emitterPos.y,
      emitterPos.z, emitterRot.x, emitterRot.y, emitterRot.z,
      emitter->GetCellOrWorld(), lookPrefix, look);
  };

  auto onUnsubscribe = [sendTarget, serverState](MpActor* emitter,
                                                 MpActor* listener) {
    auto listenerUserId = serverState->UserByActor(listener);
    if (listenerUserId != Networking::InvalidUserId)
      Networking::SendFormatted(sendTarget, listenerUserId,
                                R"({"type": "destroyActor", "idx": %u})",
                                emitter->GetIdx());
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
    auto& actor = worldState.GetFormAt<MpActor>(actorFormId);

    pImpl->serverState.actorsMap.insert({ userId, &actor });

    // Hacky way to force self-subscribing
    actor.SetPos(actor.GetPos());
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

void PartOne::SetRaceMenuOpen(uint32_t actorFormId, bool open,
                              Networking::ISendTarget* sendTarget)
{
  auto& actor = worldState.GetFormAt<MpActor>(actorFormId);

  if (actor.IsRaceMenuOpen() == open)
    return;

  actor.SetRaceMenuOpen(open);

  auto userId = pImpl->serverState.UserByActor(&actor);
  if (userId == Networking::InvalidUserId) {
    std::stringstream ss;
    ss << "Actor with id " << std::hex << actorFormId
       << " is not attached to any of users";
    throw std::runtime_error(ss.str());
  }

  Networking::SendFormatted(sendTarget, userId,
                            R"({"type": "setRaceMenuOpen", "open": %s})",
                            open ? "true" : "false");
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

MpActor* PartOne::SendToNeighbours(const simdjson::dom::element& jMessage,
                                   Networking::UserId userId,
                                   Networking::PacketData data, size_t length,
                                   bool reliable)
{
  int64_t idx;
  Read(jMessage, "idx", &idx);

  MpActor* actor = pImpl->serverState.ActorByUser(userId);

  if (actor) {
    if (idx != actor->GetIdx()) {
      std::stringstream ss;
      ss << std::hex << "You aren't able to update actor with idx " << idx
         << " (your actor's idx is " << actor->GetIdx() << ')';
      throw PublicError(ss.str());
    }

    for (auto listener : actor->GetListeners()) {
      auto targetuserId = pImpl->serverState.UserByActor(listener);

      /*
      Actually targetuserId is always valid here
      See test case in PartOneTest.cpp:

      TEST_CASE("Hypothesis: UpdateMovement may send nothing when actor
      without user present",
          "[PartOne]")
      */
      assert(targetuserId != Networking::InvalidUserId);

      pushedSendTarget->Send(targetuserId, data, length, reliable);
    }
  }

  return actor;
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
    case MsgType::UpdateAnimation: {
      SendToNeighbours(jMessage, userId, data, length);
      break;
    }

    case MsgType::UpdateLook: {
      simdjson::dom::element jData;
      Read(jMessage, "data", &jData);

      auto look = MpActor::Look::FromJson(jData);
      // TODO: validate

      auto actor = SendToNeighbours(jMessage, userId, data, length, true);

      if (actor) {
        actor->SetLook(&look);
      }
      break;
    }
    case MsgType::UpdateMovement: {
      simdjson::dom::element data_;
      Read(jMessage, "data", &data_);

      auto actor = SendToNeighbours(jMessage, userId, data, length);

      if (actor) {
        simdjson::dom::element jPos;
        Read(data_, "pos", &jPos);
        double pos[3];
        for (int i = 0; i < 3; ++i)
          Read(jPos, i, &pos[i]);
        actor->SetPos({ (float)pos[0], (float)pos[1], (float)pos[2] });
      }
      if (actor) {
        simdjson::dom::element jRot;
        Read(data_, "rot", &jRot);
        double rot[3];
        for (int i = 0; i < 3; ++i)
          Read(jRot, i, &rot[i]);
        actor->SetAngle({ (float)rot[0], (float)rot[1], (float)rot[2] });
      }
      break;
    }
    case MsgType::UpdateEquipment: {
      SendToNeighbours(jMessage, userId, data, length, true);
      break;
    }
    default:
      throw PublicError("Unknown MsgType: " + std::to_string((TypeInt)type));
  }
}