#include <Exceptions.h>
#include <JsonUtils.h>
#include <MsgType.h>
#include <PartOne.h>
#include <ServerState.h>
#include <array>
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

void PartOne::AddListener(std::shared_ptr<Listener> listener)
{
  pImpl->listeners.push_back(listener);
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