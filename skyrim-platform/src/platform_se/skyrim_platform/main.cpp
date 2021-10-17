#include "CameraApi.h"
#include "ConsoleApi.h"
#include "DevApi.h"
#include "DirectoryMonitor.h"
#include "EncodingApi.h"
#include "EventsApi.h"
#include "ExceptionPrinter.h"
#include "FlowManager.h"
#include "HttpClient.h"
#include "HttpClientApi.h"
#include "InputConverter.h"
#include "InventoryApi.h"
#include "JsEngine.h"
#include "LoadGameApi.h"
#include "MpClientPluginApi.h"
#include "SkyrimPlatformProxy.h"
#include "TaskQueue.h"
#include "ThreadPoolWrapper.h"
#include <RE/ConsoleLog.h>
#include <Windows.h>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <reverse/App.hpp>
#include <reverse/AutoPtr.hpp>
#include <reverse/Entry.hpp>
#include <shlobj.h>
#include <skse64/GameMenus.h>
#include <skse64/GameReferences.h>
#include <skse64/NiRenderer.h>
#include <skse64/gamethreads.h>
#include <sstream>
#include <string>
#include <thread>
#include <ui/MyChromiumApp.h>
#include <ui/ProcessMessageListener.h>

#include "BrowserApi.h"
#include "CallNativeApi.h"
#include <SKSE/API.h>
#include <SKSE/Interfaces.h>
#include <SKSE/Stubs.h>
#include <skse64/PluginAPI.h>

#include "SkyrimPlatform.h"

extern "C" {
__declspec(dllexport) uint32_t
  SkyrimPlatform_IpcSubscribe_Impl(const char* systemName,
                                   EventsApi::IpcMessageCallback callback,
                                   void* state)
{
  return EventsApi::IpcSubscribe(systemName, callback, state);
}

__declspec(dllexport) void SkyrimPlatform_IpcUnsubscribe_Impl(
  uint32_t subscriptionId)
{
  return EventsApi::IpcUnsubscribe(subscriptionId);
}

__declspec(dllexport) void SkyrimPlatform_IpcSend_Impl(const char* systemName,
                                                       const uint8_t* data,
                                                       uint32_t length)
{
  return EventsApi::IpcSend(systemName, data, length);
}

__declspec(dllexport) bool SKSEPlugin_Query_Impl(
  const SKSE::QueryInterface* skse, SKSE::PluginInfo* info)
{
  return SkyrimPlatform::QuerySKSEPlugin(skse, info);
}

__declspec(dllexport) bool SKSEPlugin_Load_Impl(const SKSEInterface* skse)
{
  return SkyrimPlatform::LoadSKSEPlugin(skse);
}
};

#define POINTER_SKYRIMSE(className, variableName, ...)                        \
  static CEFUtils::AutoPtr<className> variableName(__VA_ARGS__)

class SkyrimPlatformApp : public CEFUtils::SKSEPluginBase
{
public:
  static SkyrimPlatformApp& GetInstance()
  {
    static SkyrimPlatformApp g_inst;
    return g_inst;
  }

  void* GetMainAddress() const override
  {
    POINTER_SKYRIMSE(void, winMain, 0x1405ACBD0 - 0x140000000);
    return winMain.GetPtr();
  }

  bool Attach() override { return true; }

  bool Detach() override
  {
    FlowManager::CloseProcess(L"SkyrimSE.exe");
    FlowManager::CloseProcess(L"SkyrimPlatformCEF.exe");
    return true;
  }

  bool BeginMain() override
  {
    SkyrimPlatform::BeginMain();
    return true;
  }

  bool EndMain() override { return true; }

  void Update() override {}
};

DEFINE_DLL_ENTRY_INITIALIZER(SkyrimPlatformApp);
