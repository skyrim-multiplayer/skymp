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

Napi::Value MpClientPluginApi::GetVersion(const Napi::CallbackInfo &info)
{
  typedef const char* (*GetVersion)();
  auto f = (GetVersion)GetMpClientPlugin()->GetFunction("MpCommonGetVersion");
  return Napi::String::New(info.Env(), f());
}

Napi::Value MpClientPluginApi::CreateClient(const Napi::CallbackInfo &info)
{
  auto hostName = NapiHelper::ExtractString(info[0], "hostName");
  auto port = NapiHelper::ExtractInt32(info[1], "port");

  if (port < 0 || port > 65535) {
    throw std::runtime_error(std::to_string(port) + " is not a valid port");
  }

  typedef void (*CreateClient)(const char* hostName, uint16_t port);
  auto f = (CreateClient)GetMpClientPlugin()->GetFunction("CreateClient");
  f(hostName.data(), static_cast<uint16_t>(port));
  return info.Env().Undefined();
}

Napi::Value MpClientPluginApi::DestroyClient(const Napi::CallbackInfo &info)
{
  typedef void (*DestroyClient)();
  auto f = (DestroyClient)GetMpClientPlugin()->GetFunction("DestroyClient");
  f();
  return info.Env().Undefined();
}

Napi::Value MpClientPluginApi::IsConnected(const Napi::CallbackInfo &info)
{
  typedef bool (*IsConnected)();
  auto f = (IsConnected)GetMpClientPlugin()->GetFunction("IsConnected");
  return Napi::Boolean::New(info.Env(), f());
}

Napi::Value MpClientPluginApi::Tick(const Napi::CallbackInfo &info)
{
  auto onPacket = NapiHelper::ExtractFunction(env, info[0]);

  typedef void (*OnPacket)(int32_t type, const char* jsonContent,
                           const char* error, void* state);
  typedef void (*Tick)(OnPacket onPacket, void* state);

  auto f = (Tick)GetMpClientPlugin()->GetFunction("Tick");
  f(
    [](int32_t type, const char* jsonContent, const char* error, void* state) {
      auto onPacket = reinterpret_cast<Napi::Function*>(state);
      auto env = onPacket->Env();
      onPacket->Call(
        { Napi::String::New(env, GetPacketTypeName(type)), Napi::String::New(env, jsonContent), Napi::String::New(env, error) });
    },
    &onPacket);
  return info.Env().Undefined();
}

Napi::Value MpClientPluginApi::Send(const Napi::CallbackInfo &info)
{
  typedef void (*Send)(const char* jsonContent, bool reliable);

  auto jsonContent = NapiHelper::ExtractString(info[0], "jsonContent");
  auto reliable = NapiHelper::ExtractBoolean(info[1], "reliable");

  auto f = (Send)GetMpClientPlugin()->GetFunction("Send");
  f(jsonContent.data(), reliable);
  return info.Env().Undefined();
}
