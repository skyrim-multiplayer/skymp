#include "SkyrimPlatform.h"
#include "BrowserApi.h"    // APIs for register in CommonExecutionListener
#include "CallNativeApi.h" // CallNativeApi::NativeCallRequirements
#include "CameraApi.h"
#include "ConsoleApi.h" // CommonExecutionListener
#include "DevApi.h"
#include "DirectoryMonitor.h"
#include "EncodingApi.h"
#include "EventsApi.h"
#include "ExceptionPrinter.h"
#include "FileInfoApi.h"
#include "FileUtils.h"
#include "HttpClient.h"
#include "HttpClientApi.h"
#include "InventoryApi.h"
#include "LoadGameApi.h"
#include "MpClientPluginApi.h"
#include "SkyrimPlatformProxy.h"
#include "TextApi.h"
#include "ThreadPoolWrapper.h"
#include "Win32Api.h"

CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace {
bool EndsWith(const std::wstring& value, const std::wstring& ending)
{
  return ending.size() <= value.size() &&
    std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}
}

namespace {
class TickListener
{
public:
  virtual ~TickListener() = default;
  virtual void Tick() = 0;
  virtual void Update() = 0;
};

class HelloTickListener : public TickListener
{
public:
  void Tick() override
  {
    if (auto console = RE::ConsoleLog::GetSingleton()) {
      if (!helloSaid) {
        helloSaid = true;
        console->Print("Hello SE");
      }
    }
  }

  void Update() override {}

private:
  bool helloSaid = false;
};

class CommonExecutionListener : public TickListener
{
public:
  CommonExecutionListener(std::shared_ptr<BrowserApi::State> browserApiState_)
    : nativeCallRequirements(g_nativeCallRequirements)
    , browserApiState(browserApiState_)
  {
  }

  void Tick() override
  {
    try {
      GetJsEngine();

      auto fileDirs = GetFileDirs();

      if (monitors.empty()) {
        for (auto fileDir : fileDirs) {
          monitors.push_back(std::make_shared<DirectoryMonitor>(fileDir));
        }
      }

      ++tickId;

      std::vector<std::filesystem::path> pathsToLoad;

      bool hotReload = false;

      for (auto& monitor : monitors) {
        if (monitor->Updated()) {
          hotReload = true;
          // Do not break here. monitor->Updated has to be called for all
          // monitors. See method implementation
        }
        monitor->ThrowOnceIfHasError();
      }

      const bool startupLoad = tickId == 1;
      const bool loadNeeded = startupLoad || hotReload;
      if (loadNeeded) {
        ClearState();
        for (auto& fileDir : fileDirs) {
          LoadFiles(GetPathsToLoad(fileDir));
        }
      }

      HttpClientApi::GetHttpClient().ExecuteQueuedCallbacks();

      EventsApi::SendEvent("tick", {});
    } catch (const std::exception& e) {
      ExceptionPrinter::Print(e);
    }
  }

  void Update() override
  {
    try {
      GetJsEngine();
      taskQueue.Update();
      nativeCallRequirements.jsThrQ->Update();
      jsPromiseTaskQueue.Update();
      EventsApi::SendEvent("update", {});
    } catch (const std::exception& e) {
      ExceptionPrinter::Print(e);
    }
  }

private:
  std::vector<const char*> GetFileDirs() const
  {
    constexpr auto kSkympPluginsDir =
      "C:/projects/skymp/build/dist/client/Data/Platform/Plugins";
    if (std::filesystem::exists(kSkympPluginsDir)) {
      return { kSkympPluginsDir };
    }
    std::vector<const char*> dirs = { "Data/Platform/Plugins",
                                      "Data/Platform/PluginsDev" };
    return dirs;
  }

  void LoadFiles(const std::vector<std::filesystem::path>& pathsToLoad)
  {
    for (auto& path : pathsToLoad) {
      // otherwise SkyrimPlatform tries to interpret
      // skymp5-client-settings.txt.txt as a JavaScript code
      if (EndsWith(path.wstring(), L".txt.txt")) {
        logger::error("Found file with double extension: {}", path.string());
        continue;
      }
      if (EndsWith(path.wstring(), L"-settings.txt")) {
        LoadSettingsFile(path);
        continue;
      }
      if (EndsWith(path.wstring(), L"-logs.txt")) {
        continue;
      }
      if (EndsWith(path.wstring(), L".DS_Store")) {
        continue;
      }
      LoadPluginFile(path);
    }
  }

  void LoadSettingsFile(const std::filesystem::path& path)
  {
    auto s = path.filename().wstring();
    s.resize(s.size() - strlen("-settings.txt"));

    auto pluginName = std::filesystem::path(s).string();
    logger::info("Found settings file {} for plugin {}.", path.string(),
                 pluginName);

    settingsByPluginName[pluginName] = Viet::ReadFileIntoString(path);
  }

  void LoadPluginFile(const std::filesystem::path& path)
  {
    auto engine = GetJsEngine();
    auto scriptSrc = Viet::ReadFileIntoString(path);

    getSettings = [this](const JsFunctionArguments&) {
      auto result = JsValue::Object();
      auto standardJson = JsValue::GlobalObject().GetProperty("JSON");
      auto parse = standardJson.GetProperty("parse");
      for (const auto& [pluginName, settings] : settingsByPluginName) {
        result.SetProperty(pluginName, parse.Call({ standardJson, settings }));
      }
      return result;
    };

    // We will be able to use require()
    JsValue devApi = JsValue::Object();
    DevApi::Register(devApi, engine,
                     { { "skyrimPlatform",
                         [this, engine](JsValue e) {
                           EncodingApi::Register(e);
                           LoadGameApi::Register(e);
                           CameraApi::Register(e);
                           MpClientPluginApi::Register(e);
                           HttpClientApi::Register(e);
                           ConsoleApi::Register(e);
                           DevApi::Register(e, engine, {}, GetFileDirs());
                           EventsApi::Register(e);
                           BrowserApi::Register(e, browserApiState);
                           Win32Api::Register(e);
                           FileInfoApi::Register(e);
                           TextApi::Register(e);
                           InventoryApi::Register(e);
                           CallNativeApi::Register(
                             e, [this] { return nativeCallRequirements; });
                           e.SetProperty("settings", getSettings, nullptr);

                           return SkyrimPlatformProxy::Attach(
                             e, [this] { return nativeCallRequirements; });
                         } } },
                     GetFileDirs());

    JsValue consoleApi = JsValue::Object();
    ConsoleApi::Register(consoleApi);
    for (auto f : { "require", "addNativeExports" }) {
      JsValue::GlobalObject().SetProperty(f, devApi.GetProperty(f));
    }
    JsValue::GlobalObject().SetProperty(
      "log", consoleApi.GetProperty("printConsole"));

    engine->RunScript(Viet::ReadFileIntoString(
                        std::filesystem::path("Data/Platform/Distribution") /
                        "___systemPolyfill.js"),
                      "___systemPolyfill.js");
    engine->RunScript(
      "skyrimPlatform = addNativeExports('skyrimPlatform', {})", "");
    engine->RunScript(scriptSrc, path.filename().string()).ToString();
  }

  void ClearState()
  {
    ConsoleApi::Clear();
    EventsApi::Clear();
    taskQueue.Clear();
    jsPromiseTaskQueue.Clear();
    nativeCallRequirements.jsThrQ->Clear();
    settingsByPluginName.clear();
  }

  std::shared_ptr<JsEngine> GetJsEngine()
  {
    if (!engine_) {
      engine_ = std::make_shared<JsEngine>();
      engine_->ResetContext(jsPromiseTaskQueue);
    }
    return engine_;
  }

  std::vector<std::filesystem::path> GetPathsToLoad(
    const std::filesystem::path& directory)
  {
    std::vector<std::filesystem::path> paths;
    if (std::filesystem::exists(directory)) {
      for (auto& it : std::filesystem::directory_iterator(directory)) {
        std::filesystem::path p =
          it.is_directory() ? it.path() / "index.js" : it;
        paths.push_back(p);
      }
    }
    return paths;
  }

  std::shared_ptr<JsEngine> engine_;
  std::vector<std::shared_ptr<DirectoryMonitor>> monitors;
  uint32_t tickId = 0;
  Viet::TaskQueue taskQueue;
  Viet::TaskQueue jsPromiseTaskQueue;
  CallNativeApi::NativeCallRequirements& nativeCallRequirements;
  std::unordered_map<std::string, std::string> settingsByPluginName;
  std::shared_ptr<BrowserApi::State> browserApiState;
  std::function<JsValue(const JsFunctionArguments&)> getSettings;
};
}

typedef asio::executor_work_guard<asio::io_context::executor_type> SPWorkGuard;

struct SkyrimPlatform::Impl
{
  std::shared_ptr<BrowserApi::State> browserApiState;
  std::vector<std::shared_ptr<TickListener>> tickListeners;
  Viet::TaskQueue tickTasks, updateTasks;
  ThreadPoolWrapper pool;

  // Stuff needed to push functions from js to game thread
  asio::io_context ioContext;
  std::mutex syncLock;
  std::condition_variable conditionalVariable;
  bool complete;
  std::shared_ptr<SPWorkGuard> workGuard;
  void RunInIOContext(RE::BSTSmartPointer<RE::BSScript::IFunction> fPtr,
                      const RE::BSTSmartPointer<RE::BSScript::Stack>& stack,
                      RE::BSScript::ErrorLogger* logger,
                      RE::BSScript::Internal::VirtualMachine* vm,
                      RE::BSScript::IFunction::CallResult* ret)
  {
    *ret = fPtr->Call(stack, logger, vm, false);
    std::lock_guard<std::mutex> lock(syncLock);
    complete = true;
    conditionalVariable.notify_all();
  }
};

SkyrimPlatform::SkyrimPlatform()
{
  pImpl = std::make_shared<Impl>();
  pImpl->browserApiState = std::make_shared<BrowserApi::State>();

  pImpl->tickListeners.push_back(std::make_shared<HelloTickListener>());
  pImpl->tickListeners.push_back(
    std::make_shared<CommonExecutionListener>(pImpl->browserApiState));
  pImpl->complete = false;
}

SkyrimPlatform* SkyrimPlatform::GetSingleton()
{
  static SkyrimPlatform g_skyrimPlatform;
  return &g_skyrimPlatform;
}

void SkyrimPlatform::JsTick(bool gameFunctionsAvailable)
{
  for (auto& listener : pImpl->tickListeners) {
    gameFunctionsAvailable ? listener->Update() : listener->Tick();
  }

  try {
    (gameFunctionsAvailable ? pImpl->updateTasks : pImpl->tickTasks).Update();
  } catch (const std::exception& e) {
    ExceptionPrinter::Print(e);
  }
}

void SkyrimPlatform::SetOverlayService(
  std::shared_ptr<OverlayService> overlayService)
{
  pImpl->browserApiState->overlayService = overlayService;
}

void SkyrimPlatform::AddTickTask(const std::function<void()>& f)
{
  pImpl->tickTasks.AddTask(f);
}

void SkyrimPlatform::AddUpdateTask(const std::function<void()>& f)
{
  pImpl->updateTasks.AddTask(f);
}

void SkyrimPlatform::PushAndWait(const std::function<void()>& f)
{
  pImpl->pool.PushAndWait(f);
}

void SkyrimPlatform::Push(const std::function<void()>& f)
{
  pImpl->pool.Push(f);
}

void SkyrimPlatform::PushToWorkerAndWait(
  RE::BSTSmartPointer<RE::BSScript::IFunction> fPtr,
  const RE::BSTSmartPointer<RE::BSScript::Stack>& stack,
  RE::BSScript::ErrorLogger* logger,
  RE::BSScript::Internal::VirtualMachine* vm,
  RE::BSScript::IFunction::CallResult* ret)
{
  std::unique_lock<std::mutex> lock(pImpl->syncLock);
  pImpl->complete = false;
  asio::post(
    pImpl->ioContext.get_executor(),
    std::bind(&Impl::RunInIOContext, pImpl, fPtr, stack, logger, vm, ret));
  pImpl->conditionalVariable.wait(
    lock, [] { return SkyrimPlatform::GetSingleton()->pImpl->complete; });
}

void SkyrimPlatform::PrepareWorker()
{
  if (pImpl->ioContext.stopped()) {
    pImpl->ioContext.restart();
  }
  pImpl->workGuard =
    std::make_shared<SPWorkGuard>(pImpl->ioContext.get_executor());
}

void SkyrimPlatform::StartWorker()
{
  pImpl->ioContext.run();
}

void SkyrimPlatform::StopWorker()
{
  pImpl->ioContext.stop();
}
