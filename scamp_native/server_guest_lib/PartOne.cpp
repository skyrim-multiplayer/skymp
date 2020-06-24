#include "PartOne.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include "MsgType.h"
#include "ServerState.h"
#include "WorldState.h"
#include <array>
#include <optional>
#include <vector>

struct PartOne::Impl
{
  ServerState serverState;
  WorldState worldState;
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

void PartOne::AddListener(std::shared_ptr<Listener> listener)
{
  pImpl->listeners.push_back(listener);
}

void PartOne::CreateActor(uint32_t formId, const NiPoint3& pos, float angleZ,
                          uint32_t cellOrWorld, Networking::IServer* svr)
{
  pImpl->worldState.AddForm(std::unique_ptr<MpActor>(new MpActor(
                              { pos, { 0, 0, angleZ }, cellOrWorld })),
                            formId);
}

void PartOne::SetUserActor(Networking::UserId userId, uint32_t actorFormId,
                           Networking::IServer* svr)
{
  if (!pImpl->serverState.userInfo[userId]) {
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");
  }

  if (actorFormId > 0) {
    auto form = pImpl->worldState.LookupFormById(actorFormId);
    if (!form) {
      std::stringstream ss;
      ss << "Form with id " << std::hex << actorFormId << " doesn't exist";
      throw std::runtime_error(ss.str());
    }

    auto actor = dynamic_cast<MpActor*>(form);
    if (!actor) {
      std::stringstream ss;
      ss << "Form with id " << std::hex << actorFormId << " is not Actor";
      throw std::runtime_error(ss.str());
    }

    pImpl->serverState.userInfo[userId]->actor = actor;
  } else {
    pImpl->serverState.userInfo[userId]->actor = nullptr;
  }

  {
    char data[1024] = { 0 };
    data[0] = Networking::MinPacketId;
    auto len = (size_t)snprintf(data + 1, std::size(data) - 1,
                                R"({"type": "setUserActor", "formId": %u})",
                                actorFormId);
    svr->Send(userId, reinterpret_cast<Networking::PacketData>(data), len + 1,
              true);
  }

  if (MpActor* ac = pImpl->serverState.userInfo[userId]->actor) {
    auto& pos = ac->GetPos();
    auto& angle = ac->GetAngle();
    auto& cellOrWorld = ac->GetCellOrWorld();

    char data[1024] = { 0 };
    data[0] = Networking::MinPacketId;
    auto len = (size_t)snprintf(
      data + 1, std::size(data) - 1,
      R"({"type": "moveTo", "formId": %u, "pos": [%f,%f,%f], "rot": [%f,%f,%f], "cellOrWorld": %u})",
      actorFormId, pos.x, pos.y, pos.z, angle.x, angle.y, angle.z,
      cellOrWorld);
    svr->Send(userId, reinterpret_cast<Networking::PacketData>(data), len + 1,
              true);
  }
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
      this_->pImpl->serverState.Disconnect(userId);
      for (auto& listener : this_->pImpl->listeners)
        listener->OnDisconnect(userId);
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
    default:
      throw PublicError("Unknown MsgType: " + std::to_string((TypeInt)type));
  }
}