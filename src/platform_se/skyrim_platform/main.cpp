#include <Windows.h>

#include <shlobj.h>
#include <thread>

#include <RE/ConsoleLog.h>
#include <SKSE/API.h>
#include <SKSE/Interfaces.h>
#include <SKSE/Stubs.h>
#include <skse64/PluginAPI.h>
#include <skse64/gamethreads.h>

#include "DirectoryMonitor.h"
#include "JsEngine.h"
#include "MyUpdateTask.h"
#include "TaskQueue.h"

#include "ConsoleApi.h"
#include "DevApi.h"
#include "EventsApi.h"
#include "SystemPolyfill.h"
#include "VmApi.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include <ctpl/ctpl_stl.h>

#define PLUGIN_NAME "SkyrimPlatform"
#define PLUGIN_VERSION 0

void StartSKSE(void* hDllHandle);

SKSETaskInterface* g_taskInterface = nullptr;
SKSEMessagingInterface* g_messaging = nullptr;

void OnUpdate()
{
  static ctpl::thread_pool pool(1);

  auto future = pool.push([](int) {
    if (auto console = RE::ConsoleLog::GetSingleton()) {
      static bool helloSaid = false;
      if (!helloSaid) {
        helloSaid = true;
        console->Print("Hello SE");
      }
    }
    try {
      static TaskQueue taskQueue;
      taskQueue.Update();

      static std::shared_ptr<JsEngine> engine;

      auto fileDir =
        std::filesystem::path("C:/projects/skyrim-multiplayer/build/_client");
      static auto monitor = new DirectoryMonitor(fileDir);
      static uint32_t lastNumUpdates = 0;

      static uint32_t tickId = 0;
      tickId++;

      const auto n = monitor->GetNumUpdates();
      bool scriptsUpdated = false;
      if (lastNumUpdates != n) {
        lastNumUpdates = n;
        scriptsUpdated = true;
      }

      if (auto ec = monitor->GetErrorCode()) {
        static bool thrown = false;
        if (!thrown) {
          thrown = true;
          throw std::runtime_error("DirectoryMonitor failed with code " +
                                   std::to_string(ec));
        }
      }

      if (tickId == 1 || scriptsUpdated) {
        EventsApi::Clear();

        auto filePath = fileDir / "index.js";
        std::ifstream t(filePath);
        std::stringstream scriptSrc;
        scriptSrc << t.rdbuf();

        if (!engine)
          engine.reset(new JsEngine);
        engine->ResetContext(&taskQueue);

        // We will be able to use require() and log()
        JsValue devApi = JsValue::Object();
        DevApi::Register(
          devApi, &engine,
          { { "skyrimPlatform/console", ConsoleApi::Register },
            { "skyrimPlatform/dev",
              [](JsValue& e) { DevApi::Register(e, &engine, {}); } },
            { "skyrimPlatform/events", EventsApi::Register },
            { "skyrimPlatform/vm",
              [](JsValue& e) { VmApi::Register(e, &taskQueue); } } });

        JsValue consoleApi = JsValue::Object();
        ConsoleApi::Register(consoleApi);
        JsValue::GlobalObject().SetProperty("require",
                                            devApi.GetProperty("require"));
        JsValue::GlobalObject().SetProperty("log",
                                            consoleApi.GetProperty("log"));

        /*JsValue::GlobalObject().SetProperty(
          "System",
          SystemPolyfill::Register(
            &engine, [](const std::string& moduleName, JsValue& exports) {
              if (auto console = RE::ConsoleLog::GetSingleton()) {

                console->Print("moduleName %s", moduleName.data());
              }

              if (moduleName == "skyrimPlatform/console")
                ConsoleApi::Register(exports);
              else if (moduleName == "skyrimPlatform/dev")
                DevApi::Register(exports, &engine);
              else if (moduleName == "skyrimPlatform/events")
                EventsApi::Register(exports);
              else if (moduleName == "skyrimPlatform/vm")
                VmApi::Register(exports, &taskQueue);
            }));*/

        auto fileName = std::filesystem::path(filePath).filename();
        auto res =
          engine->RunScript(scriptSrc.str(), fileName.string()).ToString();
      }

      EventsApi::SendEvent("update", {});

    } catch (std::exception& e) {
      if (auto console = RE::ConsoleLog::GetSingleton()) {
        std::string what = e.what();
        std::string tmp;

        while (what.size() > sizeof("Error: ") - 1 &&
               !memcmp(what.data(), "Error: ", sizeof("Error: ") - 1)) {
          what = { what.begin() + sizeof("Error: ") - 1, what.end() };
        }

        size_t i = 0;

        auto safePrint = [what, console, &i](std::string msg) {
          if (msg.size() > 128) {
            msg.resize(128);
            msg += '...';
          }
          console->Print("%s%s", (i ? "" : "[Exception] "), msg.data());
          ++i;
        };

        for (size_t i = 0; i < what.size(); ++i) {
          if (what[i] == '\n') {
            safePrint(tmp);
            tmp.clear();
          } else {
            tmp += what[i];
          }
        }
        if (!tmp.empty())
          safePrint(tmp);
        // console->Print("[Exception] %s", e.what());
      }
    }
  });
  future.wait();
}

extern "C" {
__declspec(dllexport) bool SKSEPlugin_Query(const SKSE::QueryInterface* skse,
                                            SKSE::PluginInfo* info)
{

  info->infoVersion = SKSE::PluginInfo::kVersion;
  info->name = PLUGIN_NAME;
  info->version = PLUGIN_VERSION;

  if (skse->IsEditor()) {
    _FATALERROR("loaded in editor, marking as incompatible");
    return false;
  }
  return true;
}

__declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface* skse)
{
  g_messaging =
    (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
  if (!g_messaging) {
    _FATALERROR("couldn't get messaging interface");
    return false;
  }
  g_taskInterface = (SKSETaskInterface*)skse->QueryInterface(kInterface_Task);
  if (!g_taskInterface) {
    _FATALERROR("couldn't get task interface");
    return false;
  }

  g_taskInterface->AddTask(new MyUpdateTask(g_taskInterface, OnUpdate));

  return true;
}
};

BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      // StartSKSE(hDllHandle);
      break;

    case DLL_PROCESS_DETACH:
      break;
  };

  return TRUE;
}
