#include "CallNativeApi.h"
#include "CameraApi.h"
#include "ConsoleApi.h"
#include "DevApi.h"
#include "DirectoryMonitor.h"
#include "DumpFunctions.h"
#include "EventsApi.h"
#include "FlowManager.h"
#include "HttpClient.h"
#include "HttpClientApi.h"
#include "JsEngine.h"
#include "LoadGameApi.h"
#include "MpClientPluginApi.h"
#include "MyUpdateTask.h"
#include "PapyrusTESModPlatform.h"
#include "SkyrimPlatformProxy.h"
#include "SystemPolyfill.h"
#include "TaskQueue.h"
#include "ThreadPoolWrapper.h"
#include <RE/ConsoleLog.h>
#include <SKSE/API.h>
#include <SKSE/Interfaces.h>
#include <SKSE/Stubs.h>
#include <Windows.h>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <shlobj.h>
#include <skse64/PluginAPI.h>
#include <skse64/gamethreads.h>
#include <sstream>
#include <string>
#include <thread>

#include <skse64/GameReferences.h>

#define PLUGIN_NAME "SkyrimPlatform"
#define PLUGIN_VERSION 0

void StartSKSE(void* hDllHandle);
void SetupFridaHooks();

static SKSETaskInterface* g_taskInterface = nullptr;
static SKSEMessagingInterface* g_messaging = nullptr;
ThreadPoolWrapper g_pool;
HttpClient g_httpClient;

CallNativeApi::NativeCallRequirements g_nativeCallRequirements;
TaskQueue g_taskQueue;

std::string ReadFile(const std::filesystem::path& p)
{
  std::ifstream t(p);
  if (!t.is_open())
    throw std::runtime_error("Unable to open " + p.string() + " for reading");
  std::stringstream content;
  content << t.rdbuf();

  return content.str();
}

bool EndsWith(const std::wstring& value, const std::wstring& ending)
{
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void SafePrint(std::string what)
{
  std::string tmp;

  auto console = RE::ConsoleLog::GetSingleton();
  if (!console)
    return;

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
}

void JsTick(bool gameFunctionsAvailable)
{
  if (auto console = RE::ConsoleLog::GetSingleton()) {
    static bool helloSaid = false;
    if (!helloSaid) {
      helloSaid = true;
      console->Print("Hello SE");
    }
  }
  try {
    static std::shared_ptr<JsEngine> engine;

    auto fileDir = std::filesystem::path("Data/Platform/Plugins");
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
      ConsoleApi::Clear();
      EventsApi::Clear();
      g_taskQueue.Clear();
      g_nativeCallRequirements.jsThrQ->Clear();

      if (!engine) {
        engine.reset(new JsEngine);
        engine->ResetContext(&g_taskQueue);
      }

      thread_local JsValue g_jAllSettings = JsValue::Object();
      std::vector<std::filesystem::path> scriptsToExecute;

      for (auto& it : std::filesystem::directory_iterator(fileDir)) {

        std::filesystem::path p = it.is_directory() ? it / "index.js" : it;

        if (EndsWith(p.wstring(), L"-settings.txt")) {
          auto s = p.filename().wstring();
          s.resize(s.size() - strlen("-settings.txt"));

          auto pluginName = std::filesystem::path(s).string();
          SafePrint("Found settings file: " + p.string() + " for plugin " +
                    pluginName);

          auto standardJson = JsValue::GlobalObject().GetProperty("JSON");
          auto parsedSettings = standardJson.GetProperty("parse").Call(
            { standardJson, ReadFile(p) });
          g_jAllSettings.SetProperty(pluginName, parsedSettings);
          continue;
        }

        scriptsToExecute.push_back(p);
      }

      for (auto& scriptPath : scriptsToExecute) {
        auto scriptSrc = ReadFile(scriptPath);

        // We will be able to use require() and log()
        JsValue devApi = JsValue::Object();
        DevApi::Register(
          devApi, &engine,
          { { "skyrimPlatform",
              [fileDir](JsValue e) {
                LoadGameApi::Register(e);
                CameraApi::Register(e);
                MpClientPluginApi::Register(e);
                HttpClientApi::Register(e);
                ConsoleApi::Register(e);
                DevApi::Register(e, &engine, {}, fileDir);
                EventsApi::Register(e);
                CallNativeApi::Register(
                  e, [] { return g_nativeCallRequirements; });
                e.SetProperty(
                  "getJsMemoryUsage",
                  JsValue::Function(
                    [](const JsFunctionArguments& args) -> JsValue {
                      return (double)engine->GetMemoryUsage();
                    }));
                e.SetProperty(
                  "settings",
                  [&](const JsFunctionArguments& args) {
                    return g_jAllSettings;
                  },
                  nullptr);

                return SkyrimPlatformProxy::Attach(e);
              } } },
          fileDir);

        JsValue consoleApi = JsValue::Object();
        ConsoleApi::Register(consoleApi);
        for (auto f : { "require", "addNativeExports" })
          JsValue::GlobalObject().SetProperty(f, devApi.GetProperty(f));
        JsValue::GlobalObject().SetProperty(
          "log", consoleApi.GetProperty("printConsole"));

        engine->RunScript(
          ReadFile(std::filesystem::path("Data/Platform/Distribution") /
                   "___systemPolyfill.js"),
          "___systemPolyfill.js");
        engine->RunScript(scriptSrc, scriptPath.filename().string())
          .ToString();
      }
    }

    if (gameFunctionsAvailable) {
      g_taskQueue.Update();
      g_nativeCallRequirements.jsThrQ->Update();
    }
    if (!gameFunctionsAvailable) {
      g_httpClient.Update();
    }
    EventsApi::SendEvent(gameFunctionsAvailable ? "update" : "tick", {});

  } catch (std::exception& e) {
    if (auto console = RE::ConsoleLog::GetSingleton()) {
      std::string what = e.what();

      while (what.size() > sizeof("Error: ") - 1 &&
             !memcmp(what.data(), "Error: ", sizeof("Error: ") - 1)) {
        what = { what.begin() + sizeof("Error: ") - 1, what.end() };
      }

      SafePrint(what);
    }
  }
}

void PushJsTick(bool gameFunctionsAvailable)
{
  g_pool.Push([=](int) { JsTick(gameFunctionsAvailable); }).wait();
}

void OnUpdate()
{
  PushJsTick(false);
  TESModPlatform::Update();
}

void UpdateDumpFunctions()
{
  auto pressed = [](int key) {
    return (GetAsyncKeyState(key) & 0x80000000) > 0;
  };
  const bool comb = pressed('9') && pressed('O') && pressed('L');
  static bool g_combWas = false;

  if (comb != g_combWas) {
    g_combWas = comb;
    if (comb)
      DumpFunctions::Run();
  }
}

void OnPapyrusUpdate(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId)
{
  UpdateDumpFunctions();

  g_nativeCallRequirements.stackId = stackId;
  g_nativeCallRequirements.vm = vm;
  PushJsTick(true);
  g_nativeCallRequirements.gameThrQ->Update();
  g_nativeCallRequirements.stackId = (RE::VMStackID)~0;
  g_nativeCallRequirements.vm = nullptr;
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

  auto papyrusInterface = static_cast<SKSEPapyrusInterface*>(
    skse->QueryInterface(kInterface_Papyrus));
  if (!papyrusInterface) {
    _FATALERROR("QueryInterface failed for PapyrusInterface");
    return false;
  }

  SetupFridaHooks();

  g_taskInterface->AddTask(new MyUpdateTask(g_taskInterface, OnUpdate));

  papyrusInterface->Register(
    (SKSEPapyrusInterface::RegisterFunctions)TESModPlatform::Register);
  TESModPlatform::onPapyrusUpdate = OnPapyrusUpdate;

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
      FlowManager::CloseProcess(L"SkyrimSE.exe");
      break;
  };

  return TRUE;
}
