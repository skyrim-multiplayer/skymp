#include "MpClientPlugin.h"

#include "FileUtils.h"
#include "MessageSerializerFactory.h"
#include "MsgType.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>
#include <spdlog/spdlog.h>
#include <tuple>
#include <vector>

void MpClientPlugin::CreateClient(State& state, const char* targetHostname,
                                  uint16_t targetPort)
{
  std::string password = kNetworkingPasswordPrefix;
  // Keep in sync with installer code
  static const std::string kPasswordPath =
    "Data/Platform/Distribution/password";
  static const int kTimeoutMs = 60000;
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

    password = kNetworkingPasswordPrefix + password;
  } catch (std::exception& e) {
    spdlog::warn("Unable to read password from '{}', will use standard '{}'",
                 kPasswordPath, password.data());
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
        return onPacket(static_cast<int32_t>(packetType), "", 0, error, state);
      }

      std::string deserializedJsonContent;
      if (deserializeMessageFn(data, length, deserializedJsonContent)) {
        return onPacket(static_cast<int32_t>(packetType),
                        deserializedJsonContent.data(),
                        deserializedJsonContent.size(), error, state);
      }

      // Previously, it was string-only
      // Now it can be any bytes while still being std::string
      std::string rawContent =
        std::string(reinterpret_cast<const char*>(data) + 1, length - 1);
      onPacket(static_cast<int32_t>(packetType), rawContent.data(),
               rawContent.size(), error, state);
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

void MpClientPlugin::SendRaw(State& state, const void* data, size_t size,
                             bool reliable)
{
  if (!state.cl) {
    // TODO(#263): we probably should log something here
    return;
  }

  state.cl->Send(reinterpret_cast<Networking::PacketData>(data), size,
                 reliable);
}
