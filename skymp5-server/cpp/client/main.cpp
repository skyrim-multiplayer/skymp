#include "MpClientPlugin.h"
#include <cstdint>

namespace {
MpClientPlugin::State& GetState()
{
  static MpClientPlugin::State state;
  return state;
}
}

extern "C" {
__declspec(dllexport) const char* MpCommonGetVersion()
{
  return "0.0.1";
}

__declspec(dllexport) void CreateClient(const char* targetHostname,
                                        uint16_t targetPort)
{
  return MpClientPlugin::CreateClient(GetState(), targetHostname, targetPort);
}

__declspec(dllexport) void DestroyClient()
{
  return MpClientPlugin::DestroyClient(GetState());
}

__declspec(dllexport) bool IsConnected()
{
  return MpClientPlugin::IsConnected(GetState());
}

__declspec(dllexport) void Tick(MpClientPlugin::OnPacket onPacket, void* state)
{

  return MpClientPlugin::Tick(GetState(), onPacket, state);
}

__declspec(dllexport) void Send(const char* jsonContent, bool reliable)
{
  return MpClientPlugin::Send(GetState(), jsonContent, reliable);
}

__declspec(dllexport) bool SKSEPlugin_Query(void* skse, void* info)
{
  struct PluginInfo
  {
    uint32_t infoVersion = 1;
    const char* name = "MpClientPlugin";
    uint32_t version = 1;
  };
  new (info) PluginInfo;
  return true;
}

__declspec(dllexport) bool SKSEPlugin_Load(void* skse)
{
  return true;
}
}
