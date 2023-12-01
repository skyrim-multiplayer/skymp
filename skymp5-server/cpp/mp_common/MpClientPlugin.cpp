#include "MpClientPlugin.h"

#include "FileUtils.h"
#include "MessageSerializerFactory.h"
#include "MovementMessage.h"
#include "MsgType.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>
#include <spdlog/spdlog.h>
#include <tuple>
#include <vector>

void MpClientPlugin::CreateClient(State& state, const char* targetHostname,
                                  uint16_t targetPort)
{
  std::string password = kNetworkingPassword;
  // Keep in sync with installer code
  static const std::string kPasswordPath =
    "Data/Platform/Distribution/password";
  static const int kTimeoutMs = 4000;
  try {
    password = Viet::ReadFileIntoString(kPasswordPath);

    // Remove trailing Windows-style newlines (\r\n)
    while (password.size() >= 2 && password[password.length() - 2] == '\r' &&
           password[password.length() - 1] == '\n') {
      password.erase(password.length() - 2);
    }

    // Remove trailing Unix-style newlines (\n)
    while (!password.empty() && password.back() == '\n') {
      password.pop_back();
    }
  } catch (std::exception& e) {
    spdlog::warn("Unable to read password from '{}', will use standard '{}'",
                 kPasswordPath, kNetworkingPassword);
  }
  state.cl = Networking::CreateClient(targetHostname, targetPort, kTimeoutMs,
                                      password.data());
}

void MpClientPlugin::DestroyClient(State& state)
{
  state.cl.reset();
}

bool MpClientPlugin::IsConnected(State& state)
{
  return state.cl && state.cl->IsConnected();
}

void MpClientPlugin::Tick(State& state, OnPacket onPacket,
                          DeserializeMessage deserializeMessageFn,
                          void* state_)
{
  if (!state.cl)
    return;

  std::tuple<OnPacket, DeserializeMessage, void*> locals(
    onPacket, deserializeMessageFn, state_);

  state.cl->Tick(
    [](void* rawState, Networking::PacketType packetType,
       Networking::PacketData data, size_t length, const char* error) {
      const auto& [onPacket, deserializeMessageFn, state] =
        *reinterpret_cast<std::tuple<OnPacket, DeserializeMessage, void*>*>(
          rawState);

      if (packetType != Networking::PacketType::Message) {
        return onPacket(static_cast<int32_t>(packetType), "", error, state);
      }

      std::string deserializedJsonContent;
      if (deserializeMessageFn(data, length, deserializedJsonContent)) {
        return onPacket(static_cast<int32_t>(packetType),
                        deserializedJsonContent.data(), error, state);
      }

      std::string jsonContent =
        std::string(reinterpret_cast<const char*>(data) + 1, length - 1);
      onPacket(static_cast<int32_t>(packetType), jsonContent.data(), error,
               state);
    },
    &locals);
}

void MpClientPlugin::Send(State& state, const char* jsonContent, bool reliable,
                          SerializeMessage serializeMessageFn)
{
  if (!state.cl) {
    // TODO(#263): we probably should log something here
    return;
  }

  SLNet::BitStream stream;
  serializeMessageFn(jsonContent, stream);
  state.cl->Send(stream.GetData(), stream.GetNumberOfBytesUsed(), reliable);
}
