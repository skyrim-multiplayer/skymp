#include "PartOne.h"
#include "Exceptions.h"
#include "IdManager.h"
#include "JsonUtils.h"
#include "MsgType.h"
#include <array>
#include <cassert>
#include <type_traits>
#include <vector>

struct PartOne::Impl
{
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
  worldState.Clear();
  serverState = {};
}

void PartOne::AddListener(std::shared_ptr<Listener> listener)
{
  pImpl->listeners.push_back(listener);
}

bool PartOne::IsConnected(Networking::UserId userId) const
{
  return serverState.IsConnected(userId);
}

void PartOne::CreateActor(uint32_t formId, const NiPoint3& pos, float angleZ,
                          uint32_t cellOrWorld,
                          Networking::ISendTarget* sendTarget)
{
  auto st = &serverState;

  auto onSubscribe = [sendTarget, st](MpActor* emitter, MpActor* listener) {
    auto& emitterPos = emitter->GetPos();
    auto& emitterRot = emitter->GetAngle();

    bool isMe = emitter == listener;

    const char *lookPrefix = "", *look = "";
    auto& jLook = emitter->GetLookAsJson();
    if (!jLook.empty()) {
      lookPrefix = R"(, "look": )";
      look = jLook.data();
    }

    const char *equipmentPrefix = "", *equipment = "";
    auto& jEquipment = emitter->GetEquipmentAsJson();
    if (!jEquipment.empty()) {
      equipmentPrefix = R"(, "equipment": )";
      equipment = jEquipment.data();
    }

    auto listenerUserId = st->UserByActor(listener);
    if (listenerUserId != Networking::InvalidUserId)
      Networking::SendFormatted(
        sendTarget, listenerUserId,
        R"({"type": "createActor", "idx": %u, "isMe": %s, "transform": {"pos":
    [%f,%f,%f], "rot": [%f,%f,%f], "worldOrCell": %u}%s%s%s%s})",
        emitter->GetIdx(), isMe ? "true" : "false", emitterPos.x, emitterPos.y,
        emitterPos.z, emitterRot.x, emitterRot.y, emitterRot.z,
        emitter->GetCellOrWorld(), lookPrefix, look, equipmentPrefix,
        equipment);
  };

  auto onUnsubscribe = [sendTarget, st](MpActor* emitter, MpActor* listener) {
    auto listenerUserId = st->UserByActor(listener);
    if (listenerUserId != Networking::InvalidUserId &&
        listenerUserId != st->disconnectingUserId)
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
  serverState.EnsureUserExists(userId);

  if (actorFormId > 0) {
    auto& actor = worldState.GetFormAt<MpActor>(actorFormId);

    serverState.actorsMap.insert({ userId, &actor });

    // Hacky way to force self-subscribing
    actor.SetPos(actor.GetPos());
  } else {
    serverState.actorsMap.left.erase(userId);
  }
}

uint32_t PartOne::GetUserActor(Networking::UserId userId)
{
  serverState.EnsureUserExists(userId);

  auto actor = serverState.ActorByUser(userId);
  if (!actor)
    return 0;
  return actor->GetFormId();
}

void PartOne::DestroyActor(uint32_t actorFormId)
{
  std::shared_ptr<MpActor> destroyedForm;
  worldState.DestroyForm<MpActor>(actorFormId, &destroyedForm);

  serverState.actorsMap.right.erase(destroyedForm.get());
}

void PartOne::SetRaceMenuOpen(uint32_t actorFormId, bool open,
                              Networking::ISendTarget* sendTarget)
{
  auto& actor = worldState.GetFormAt<MpActor>(actorFormId);

  if (actor.IsRaceMenuOpen() == open)
    return;

  actor.SetRaceMenuOpen(open);

  auto userId = serverState.UserByActor(&actor);
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

void PartOne::SendCustomPacket(Networking::UserId userId,
                               const std::string& jContent,
                               Networking::ISendTarget* sendTarget)
{
  Networking::SendFormatted(sendTarget, userId,
                            R"({"type": "customPacket", "content":%s})",
                            jContent.data());
}

std::string PartOne::GetActorName(uint32_t actorFormId)
{
  auto& ac = worldState.GetFormAt<MpActor>(actorFormId);
  return ac.GetLook() ? ac.GetLook()->name : "Prisoner";
}

NiPoint3 PartOne::GetActorPos(uint32_t actorFormId)
{
  auto& ac = worldState.GetFormAt<MpActor>(actorFormId);
  return ac.GetPos();
}

Networking::UserId PartOne::ConnectBot()
{
  return -1;
}

void PartOne::DisconnectBot(Networking::UserId id)
{
}

namespace {
class ScopedTask
{
public:
  ScopedTask(std::function<void()> f_)
    : f(f_)
  {
  }
  ~ScopedTask() { f(); }

private:
  const std::function<void()> f;
};
}

void PartOne::HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length)
{
  auto this_ = reinterpret_cast<PartOne*>(partOneInstance);

  switch (packetType) {
    case Networking::PacketType::ServerSideUserConnect:
      return this_->AddUser(userId, UserType::User);
    case Networking::PacketType::ServerSideUserDisconnect: {
      ScopedTask t([userId, this_] {
        this_->serverState.Disconnect(userId);
        this_->serverState.disconnectingUserId = Networking::InvalidUserId;
      });

      this_->serverState.disconnectingUserId = userId;
      for (auto& listener : this_->pImpl->listeners)
        listener->OnDisconnect(userId);
      return;
    }
    case Networking::PacketType::Message:
      return this_->HandleMessagePacket(userId, data, length);
    default:
      throw std::runtime_error("Unexpected PacketType: " +
                               std::to_string((int)packetType));
  }
}

void PartOne::AddUser(Networking::UserId userId, UserType type)
{
  serverState.Connect(userId);
  for (auto& listener : pImpl->listeners)
    listener->OnConnect(userId);
}

MpActor* PartOne::SendToNeighbours(const simdjson::dom::element& jMessage,
                                   Networking::UserId userId,
                                   Networking::PacketData data, size_t length,
                                   bool reliable)
{
  int64_t idx;
  Read(jMessage, "idx", &idx);

  MpActor* actor = serverState.ActorByUser(userId);

  if (actor) {
    if (idx != actor->GetIdx()) {
      std::stringstream ss;
      ss << std::hex << "You aren't able to update actor with idx " << idx
         << " (your actor's idx is " << actor->GetIdx() << ')';
      throw PublicError(ss.str());
    }

    for (auto listener : actor->GetListeners()) {
      auto targetuserId = serverState.UserByActor(listener);
      if (targetuserId != Networking::InvalidUserId)
        pushedSendTarget->Send(targetuserId, data, length, reliable);
    }
  }

  return actor;
}

void PartOne::HandleMessagePacket(Networking::UserId userId,
                                  Networking::PacketData data, size_t length)
{
  if (!serverState.IsConnected(userId))
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");
  auto& user = *serverState.userInfo[userId];

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
      auto actor = SendToNeighbours(jMessage, userId, data, length, true);
      if (actor) {
        simdjson::dom::element data_;
        Read(jMessage, "data", &data_);
        actor->SetEquipment(simdjson::minify(data_));
      }
      break;
    }
    default:
      throw PublicError("Unknown MsgType: " + std::to_string((TypeInt)type));
  }
}