#include "MpClientPluginApi.h"

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

class MpClientPlugin
{
public:
  MpClientPlugin()
  {
    hModule = LoadLibraryA("Data/SKSE/Plugins/MpClientPlugin.dll");
    if (!hModule) {
      throw std::runtime_error("Unable to load MpClientPlugin, error code " +
                               std::to_string(GetLastError()));
    }
  }

  void* GetFunction(const char* funcName)
  {
    auto addr = GetProcAddress(hModule, funcName);
    if (!addr) {
      throw std::runtime_error(
        "Unable to find function with name '" + std::string(funcName) +
        "' in MpClientPlugin, error code " + std::to_string(GetLastError()));
    }
    return addr;
  }

  HMODULE hModule = nullptr;
};

MpClientPlugin* GetMpClientPlugin()
{
  static MpClientPlugin instance;
  return &instance;
}
}

JsValue MpClientPluginApi::GetVersion(const JsFunctionArguments& args)
{
  typedef const char* (*GetVersion)();
  auto f = (GetVersion)GetMpClientPlugin()->GetFunction("MpCommonGetVersion");
  return JsValue::String(f());
}

JsValue MpClientPluginApi::CreateClient(const JsFunctionArguments& args)
{
  auto hostName = (std::string)args[1];
  auto port = (int)args[2];

  if (port < 0 || port > 65535)
    throw std::runtime_error(std::to_string(port) + " is not a valid port");

  typedef void (*CreateClient)(const char* hostName, uint16_t port);
  auto f = (CreateClient)GetMpClientPlugin()->GetFunction("CreateClient");
  f(hostName.data(), (uint16_t)port);
  return JsValue::Undefined();
}

JsValue MpClientPluginApi::DestroyClient(const JsFunctionArguments& args)
{
  typedef void (*DestroyClient)();
  auto f = (DestroyClient)GetMpClientPlugin()->GetFunction("DestroyClient");
  f();
  return JsValue::Undefined();
}

JsValue MpClientPluginApi::IsConnected(const JsFunctionArguments& args)
{
  typedef bool (*IsConnected)();
  auto f = (IsConnected)GetMpClientPlugin()->GetFunction("IsConnected");
  return JsValue::Bool(f());
}

JsValue MpClientPluginApi::Tick(const JsFunctionArguments& args)
{
  auto onPacket = args[1];

  typedef void (*OnPacket)(int32_t type, const char* jsonContent,
                           const char* error, void* state);
  typedef void (*Tick)(OnPacket onPacket, void* state);

  auto f = (Tick)GetMpClientPlugin()->GetFunction("Tick");
  f(
    [](int32_t type, const char* jsonContent, const char* error, void* state) {
      auto onPacket = reinterpret_cast<JsValue*>(state);
      onPacket->Call(
        { JsValue::Undefined(), GetPacketTypeName(type), jsonContent, error });
    },
    &onPacket);
  return JsValue::Undefined();
}

JsValue MpClientPluginApi::Send(const JsFunctionArguments& args)
{
  typedef void (*Send)(const char* jsonContent, bool reliable);

  auto jsonContent = (std::string)args[1];
  auto reliable = (bool)args[2];

  auto f = (Send)GetMpClientPlugin()->GetFunction("Send");
  f(jsonContent.data(), reliable);
  return JsValue::Undefined();
}
