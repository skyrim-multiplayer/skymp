#ifdef _SP_WITH_NETWORKING_CLIENT
#  include "NetworkingClientApi.h"
#  include "NetworkingClient.h"

namespace {
const char* GetPacketTypeName(int32_t type)
{
  switch (type) {
    case 0:
      return "message";
    case 1:
      return "disconnect";
    case 2:
      return "connectionAccepted";
    case 3:
      return "connectionFailed";
    case 4:
      return "connectionDenied";
    default:
      return "";
  }
}
}

JsValue NetworkingClientApi::Create(const JsFunctionArguments& args)
{
  auto hostName = (std::string)args[1];
  auto port = (int)args[2];

  if (port < 0 || port > 65535)
    throw std::runtime_error(std::to_string(port) + " is not a valid port");

  NetworkingClient::Create(hostName.data(), port);
  return JsValue::Undefined();
}

JsValue NetworkingClientApi::Destroy(const JsFunctionArguments& args)
{
  NetworkingClient::Destroy();
  return JsValue::Undefined();
}

JsValue NetworkingClientApi::IsConnected(const JsFunctionArguments& args)
{
  bool res = NetworkingClient::IsConnected();
  return JsValue::Bool(res);
}

JsValue NetworkingClientApi::HandlePackets(const JsFunctionArguments& args)
{
  auto onPacket = args[1];
  NetworkingClient::HandlePackets(
    [](int32_t type, const char* jsonContent, const char* error, void* state) {
      auto onPacket = reinterpret_cast<JsValue*>(state);
      onPacket->Call(
        { JsValue::Undefined(), GetPacketTypeName(type), jsonContent, error });
    },
    &onPacket);
  return JsValue::Undefined();
}

JsValue NetworkingClientApi::Send(const JsFunctionArguments& args)
{
  auto jsonContent = (std::string)args[1];
  auto reliable = (bool)args[2];

  NetworkingClient::Send(jsonContent.data(), reliable);
  return JsValue::Undefined();
}
#endif
