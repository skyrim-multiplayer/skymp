#include "ExecutionCommonsListener.h"
#include "BrowserApi.h"
#include "CallNativeApi.h"
#include "DirectoryMonitor.h"
#include "JsEngine.h"

// APIs for register
#include "BrowserApi.h"
#include "CallNativeApi.h"
#include "CameraApi.h"
#include "ConsoleApi.h"
#include "DevApi.h"
#include "EncodingApi.h"
#include "EventsApi.h"
#include "HttpClientApi.h"
#include "InventoryApi.h"
#include "LoadGameApi.h"
#include "MpClientPluginApi.h"
#include "NullPointerException.h"

// For other needs
#include "ConsoleApi.h"
#include "DirectoryMonitor.h"
#include "EventsApi.h"
#include "ExceptionPrinter.h"
#include "HttpClient.h"
#include "ReadFile.h"
#include "SkyrimPlatformProxy.h"

struct ExecutionCommonsListener::Impl
{
  std::shared_ptr<JsEngine> engine;
  std::shared_ptr<DirectoryMonitor> monitor;
  uint32_t tickId = 0;
  TaskQueue taskQueue;
  TaskQueue jsPromiseTaskQueue;
  std::shared_ptr<CallNativeApi::NativeCallRequirements>
    nativeCallRequirements;
  std::unordered_map<std::string, std::string> settingsByPluginName;
  std::shared_ptr<BrowserApi::State> browserApiState =
    std::make_shared<BrowserApi::State>();
  std::function<JsValue(const JsFunctionArguments&)> getSettings;
};

ExecutionCommonsListener::ExecutionCommonsListener(
  const std::shared_ptr<CallNativeApi::NativeCallRequirements>&
    nativeCallRequirements)
{
  if (!nativeCallRequirements) {
    throw NullPointerException("nativeCallRequirements");
  }

  pImpl = std::make_shared<Impl>();
  pImpl->nativeCallRequirements = nativeCallRequirements;
}

void ExecutionCommonsListener::Tick()
{
  PerformHotReload();

  HttpClientApi::GetHttpClient().ExecuteQueuedCallbacks();

  EventsApi::SendEvent("tick", {});
}

void ExecutionCommonsListener::Update()
{
  // TODO(#376): Hot reload strictly in non-game context
  PerformHotReload();

  pImpl->taskQueue.Update();
  pImpl->nativeCallRequirements->jsThrQ->Update();
  pImpl->jsPromiseTaskQueue.Update();
  EventsApi::SendEvent("update", {});
}

void ExecutionCommonsListener::BeginMain()
{
}

void ExecutionCommonsListener::EndMain()
{
}

// TODO(#332): Enable hot reloading for the /data/Platform/UI folder
void ExecutionCommonsListener::PerformHotReload()
{
  auto fileDir = GetFileDir();

  if (!pImpl->monitor) {
    pImpl->monitor = std::make_shared<DirectoryMonitor>(fileDir);
  }

  ++pImpl->tickId;

  bool pluginsDirectoryUpdated = pImpl->monitor->Updated();
  pImpl->monitor->ThrowOnceIfHasError();
  if (pImpl->tickId == 1 || pluginsDirectoryUpdated) {
    ClearState();
    LoadFiles(GetPathsToLoad(fileDir));
  }
}

const char* ExecutionCommonsListener::GetFileDir() const
{
  return "Data/Platform/Plugins";
}

// TODO(#306): Improve error message for incorrect client settings
void ExecutionCommonsListener::LoadFiles(
  const std::vector<std::filesystem::path>& pathsToLoad)
{
  auto& engine = GetJsEngine();

  for (auto& path : pathsToLoad) {
    if (EndsWith(path.wstring(), L"-settings.txt")) {
      LoadSettingsFile(path);
      continue;
    }
    if (EndsWith(path.wstring(), L"-logs.txt")) {
      continue;
    }
    LoadPluginFile(path);
  }
}

void ExecutionCommonsListener::LoadSettingsFile(
  const std::filesystem::path& path)
{
  auto s = path.filename().wstring();
  s.resize(s.size() - strlen("-settings.txt"));

  auto pluginName = std::filesystem::path(s).string();

  // TODO(#378): Print "Found settings file" message as non-exception
  std::string what =
    "Found settings file: " + path.string() + " for plugin " + pluginName;
  ExceptionPrinter(ConsoleApi::GetExceptionPrefix())
    .PrintException(what.data());

  pImpl->settingsByPluginName[pluginName] = ReadFile(path);
}

void ExecutionCommonsListener::LoadPluginFile(
  const std::filesystem::path& path)
{
  auto scriptSrc = ReadFile(path);

  pImpl->getSettings = [this](const JsFunctionArguments&) {
    auto result = JsValue::Object();
    auto standardJson = JsValue::GlobalObject().GetProperty("JSON");
    auto parse = standardJson.GetProperty("parse");
    for (auto [pluginName, settings] : pImpl->settingsByPluginName) {
      result.SetProperty(pluginName, parse.Call({ standardJson, settings }));
    }
    return result;
  };

  // We will be able to use require()
  // Seems that DevApi::Register does not require engine itself, but a
  // placeholder where engine may appear in the future. That's why here is
  // pointer to a shared pointer.
  JsValue devApi = JsValue::Object();
  DevApi::Register(devApi, &pImpl->engine,
                   { { "skyrimPlatform",
                       [this](JsValue e) {
                         EncodingApi::Register(e);
                         LoadGameApi::Register(e);
                         CameraApi::Register(e);
                         MpClientPluginApi::Register(e);
                         HttpClientApi::Register(e);
                         ConsoleApi::Register(e);
                         DevApi::Register(e, &pImpl->engine, {}, GetFileDir());
                         EventsApi::Register(e);
                         BrowserApi::Register(e, pImpl->browserApiState);
                         InventoryApi::Register(e);
                         CallNativeApi::Register(e, [this] {
                           return *pImpl->nativeCallRequirements;
                         });
                         e.SetProperty("settings", pImpl->getSettings,
                                       nullptr);

                         return SkyrimPlatformProxy::Attach(e);
                       } } },
                   GetFileDir());

  JsValue consoleApi = JsValue::Object();
  ConsoleApi::Register(consoleApi);
  for (auto f : { "require", "addNativeExports" }) {
    JsValue::GlobalObject().SetProperty(f, devApi.GetProperty(f));
  }
  JsValue::GlobalObject().SetProperty("log",
                                      consoleApi.GetProperty("printConsole"));

  GetJsEngine().RunScript(
    ReadFile(std::filesystem::path("Data/Platform/Distribution") /
             "___systemPolyfill.js"),
    "___systemPolyfill.js");
  GetJsEngine().RunScript(scriptSrc, path.filename().string()).ToString();
}

void ExecutionCommonsListener::ClearState()
{
  ConsoleApi::Clear();
  EventsApi::Clear();
  pImpl->taskQueue.Clear();
  pImpl->jsPromiseTaskQueue.Clear();
  pImpl->nativeCallRequirements->jsThrQ->Clear();
  pImpl->settingsByPluginName.clear();
}

JsEngine& ExecutionCommonsListener::GetJsEngine()
{
  if (!pImpl->engine) {
    pImpl->engine = std::make_shared<JsEngine>();
    pImpl->engine->ResetContext(pImpl->jsPromiseTaskQueue);
  }
  return *pImpl->engine;
}

bool ExecutionCommonsListener::EndsWith(const std::wstring& value,
                                        const std::wstring& ending)
{
  if (ending.size() > value.size()) {
    return false;
  }
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::vector<std::filesystem::path> ExecutionCommonsListener::GetPathsToLoad(
  const std::filesystem::path& directory)
{
  std::vector<std::filesystem::path> paths;
  for (auto& it : std::filesystem::directory_iterator(directory)) {
    std::filesystem::path p = it.is_directory() ? it / "index.js" : it;
    paths.push_back(p);
  }
  return paths;
}