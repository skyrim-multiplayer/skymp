#include "SkyrimPlatform.h"
#include "BrowserApi.h"    // APIs for register in CommonExecutionListener
#include "CallNativeApi.h" // CallNativeApi::NativeCallRequirements
#include "CameraApi.h"
#include "ConsoleApi.h" // CommonExecutionListener
#include "ConstEnumApi.h"
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
#include "MagicApi.h"
#include "MpClientPluginApi.h"
#include "ObjectReferenceApi.h"
#include "Sp3Api.h"
#include "TextApi.h"
#include "ThreadPoolWrapper.h"
#include "Win32Api.h"

#include "NapiHelper.h"

#include "IPC.h" // IPC::Call

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
  friend class SkyrimPlatform;

public:
  CommonExecutionListener(std::shared_ptr<BrowserApi::State> browserApiState_)
    : nativeCallRequirements(g_nativeCallRequirements)
    , browserApiState(browserApiState_)
  {
  }

  void Tick() override
  {
    try {
      GetJsEngine()->AcquireEnvAndCall(
        [this](Napi::Env env) { TickImpl(env); });
    } catch (const std::exception& e) {
      ExceptionPrinter::Print(e);
    }
  }

  void Update() override
  {
    try {
      GetJsEngine()->AcquireEnvAndCall(
        [this](Napi::Env env) { UpdateImpl(env); });
    } catch (const std::exception& e) {
      ExceptionPrinter::Print(e);
    }
  }

private:
  void TickImpl(Napi::Env env)
  {
    auto& fileDirs = GetFileDirs();

    if (monitors.empty()) {
      for (auto& fileDir : fileDirs) {
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
        LoadFiles(env, GetPathsToLoad(fileDir));
      }
    }

    HttpClientApi::GetHttpClient().ExecuteQueuedCallbacks(env);

    EventsApi::SendEvent("tick", {});
  }

  void UpdateImpl(Napi::Env env)
  {
    taskQueue.Update(env);
    nativeCallRequirements.jsThrQ->Update(env);
    jsPromiseTaskQueue.Update(env);
    EventsApi::SendEvent("update", {});
  }

  const std::vector<std::filesystem::path>& GetFileDirs() const
  {
    if (!pluginFolders) {
      try {
        pluginFolders = Settings::GetPlatformSettings()->GetPluginFolders();
      } catch (std::exception& e) {
        pluginFolders = std::make_unique<std::vector<std::filesystem::path>>();
        throw;
      }
    }

    return *pluginFolders;
  }

  void LoadFiles(Napi::Env env,
                 const std::vector<std::filesystem::path>& pathsToLoad)
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
      if (EndsWith(path.wstring(), L".js")) {
        LoadPluginFile(env, path);
        continue;
      }
      logger::warn("Found unprocessed file: {}", path.string());
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
    settingsByPluginNameCache.reset();
  }

  static void OnLoadPluginFileCallback(CommonExecutionListener& self,
                                       const char* patchedSourceCode,
                                       uint32_t patchedSourceCodeLength)
  {
    self.tmpPatchedPluginSources =
      std::string{ patchedSourceCode,
                   patchedSourceCode + patchedSourceCodeLength };
  }

  std::string PatchPluginFile(const std::filesystem::path& path,
                              const std::string& scriptSrc)
  {
#pragma pack(push, 1)
    struct Msg
    {
      void* onLoadPluginFileCallback = nullptr;
      void* self = nullptr;
      const char* pluginPathUtf8 = nullptr;
      const char* scriptSrc = nullptr;
    };
#pragma pack(pop)
    static_assert(sizeof(Msg) == 32);

    auto s = path.u8string();
    std::string pluginPathUtf8(s.begin(), s.end());

    Msg msg;
    msg.onLoadPluginFileCallback = OnLoadPluginFileCallback;
    msg.self = this;
    msg.pluginPathUtf8 = pluginPathUtf8.data();
    msg.scriptSrc = scriptSrc.data();

    tmpPatchedPluginSources = std::nullopt;

    IPC::Call("SkyrimPlatform_OnLoadPluginFile",
              reinterpret_cast<uint8_t*>(&msg), sizeof(Msg));

    if (tmpPatchedPluginSources != std::nullopt) {
      return *tmpPatchedPluginSources;
    }

    return scriptSrc;
  }

  void LoadPluginFile(Napi::Env env, const std::filesystem::path& path)
  {
    auto scriptSrc = Viet::ReadFileIntoString(path);

    scriptSrc = PatchPluginFile(path, scriptSrc);

    getSettings = [this](const Napi::CallbackInfo& info) {
      if (!settingsByPluginNameCache) {
        auto result = Napi::Object::New(info.Env());
        for (const auto& [pluginName, settings] : settingsByPluginName) {
          auto parsedSettings = NapiHelper::ParseJson(info.Env(), settings);
          result.Set(pluginName, parsedSettings);
        }
        settingsByPluginNameCache.reset(
          new Napi::Reference<Napi::Object>(Napi::Persistent(result)));
      }
      return (*settingsByPluginNameCache).Value();
    };

    setSettings = [](const Napi::CallbackInfo& info) -> Napi::Value {
      throw std::runtime_error("settings setter not implemented");
      return info.Env().Undefined();
    };

    // We will be able to use require()
    auto devApi = Napi::Object::New(env);

    DevApi::NativeExportsMap nativeExportsMap;

    nativeExportsMap["skyrimPlatform"] = [this](Napi::Object e) {
      auto getNativeCallRequirements = [this] {
        return nativeCallRequirements;
      };
      auto env = e.Env();
      auto engine = GetJsEngine();
      EncodingApi::Register(env, e);
      LoadGameApi::Register(env, e);
      CameraApi::Register(env, e);
      MpClientPluginApi::Register(env, e);
      ObjectReferenceApi::Register(env, e);
      HttpClientApi::Register(env, e);
      ConsoleApi::Register(env, e);
      DevApi::Register(env, e, {}, GetFileDirs());
      EventsApi::Register(env, e);
      BrowserApi::Register(env, e, browserApiState);
      Win32Api::Register(env, e);
      FileInfoApi::Register(env, e);
      TextApi::Register(env, e);
      InventoryApi::Register(env, e);
      MagicApi::Register(env, e);
      ConstEnumApi::Register(env, e);
      CallNativeApi::Register(env, e, getNativeCallRequirements);
      Sp3Api::Register(env, e, getNativeCallRequirements);

      auto getter = NapiHelper::WrapCppExceptions(getSettings);
      auto setter = NapiHelper::WrapCppExceptions(setSettings);

      Napi::PropertyDescriptor settingsProperty =
        Napi::PropertyDescriptor::Accessor("settings", getter, setter);
      e.DefineProperty(settingsProperty);

      return e;
    };

    auto engine = GetJsEngine();
    DevApi::Register(env, devApi, nativeExportsMap, GetFileDirs());

    Napi::Object consoleApi = Napi::Object::New(env);
    ConsoleApi::Register(env, consoleApi);

    // I commented out require, because NodeJS has its own require function
    // I think it's the part of systemjs polyfill and it should be removed
    for (auto f : { /*"require", */ "addNativeExports" }) {
      env.Global().Set(f, devApi.Get(f));
    }
    env.Global().Set("log", consoleApi.Get("printConsole"));

    engine->RunScript(env,
                      Viet::ReadFileIntoString(
                        std::filesystem::path("Data/Platform/Distribution") /
                        "___systemPolyfill.js"),
                      "___systemPolyfill.js");
    engine->RunScript(
      env, "skyrimPlatform = addNativeExports('skyrimPlatform', {})", "");
    engine->RunScript(env, scriptSrc, path.filename().string()).ToString();
  }

  void ClearState()
  {
    ConsoleApi::Clear();
    EventsApi::Clear();
    taskQueue.Clear();
    jsPromiseTaskQueue.Clear();
    nativeCallRequirements.jsThrQ->Clear();
    settingsByPluginName.clear();
    settingsByPluginNameCache.reset();
  }

  JsEngine* GetJsEngine() { return JsEngine::GetSingleton(); }

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

  std::vector<std::shared_ptr<DirectoryMonitor>> monitors;
  uint32_t tickId = 0;
  Viet::TaskQueue<Napi::Env> taskQueue;
  Viet::TaskQueue<Napi::Env> jsPromiseTaskQueue;
  CallNativeApi::NativeCallRequirements& nativeCallRequirements;
  std::unordered_map<std::string, std::string> settingsByPluginName;
  std::unique_ptr<Napi::Reference<Napi::Object>> settingsByPluginNameCache;
  std::shared_ptr<BrowserApi::State> browserApiState;
  std::function<Napi::Value(const Napi::CallbackInfo& info)> getSettings,
    setSettings;
  mutable std::unique_ptr<std::vector<std::filesystem::path>> pluginFolders;
  std::optional<std::string> tmpPatchedPluginSources;
};
}

typedef asio::executor_work_guard<asio::io_context::executor_type> SPWorkGuard;

struct SkyrimPlatform::Impl
{
  std::shared_ptr<CommonExecutionListener> commonExecutionListener;
  std::shared_ptr<BrowserApi::State> browserApiState;
  std::vector<std::shared_ptr<TickListener>> tickListeners;
  Viet::TaskQueue<Napi::Env> tickTasks, updateTasks;
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

  pImpl->commonExecutionListener =
    std::make_shared<CommonExecutionListener>(pImpl->browserApiState);

  pImpl->tickListeners.push_back(std::make_shared<HelloTickListener>());
  pImpl->tickListeners.push_back(pImpl->commonExecutionListener);
  pImpl->complete = false;
}

SkyrimPlatform* SkyrimPlatform::GetSingleton()
{
  static SkyrimPlatform g_skyrimPlatform;
  return &g_skyrimPlatform;
}

void SkyrimPlatform::JsTick(Napi::Env env, bool gameFunctionsAvailable)
{
  if (gameFunctionsAvailable) {
    pImpl->commonExecutionListener->GetJsEngine()->Tick();
  }

  for (auto& listener : pImpl->tickListeners) {
    gameFunctionsAvailable ? listener->Update() : listener->Tick();
  }

  try {
    (gameFunctionsAvailable ? pImpl->updateTasks : pImpl->tickTasks)
      .Update(env);
  } catch (const std::exception& e) {
    ExceptionPrinter::Print(e);
  }
}

void SkyrimPlatform::SetOverlayService(
  std::shared_ptr<OverlayService> overlayService)
{
  pImpl->browserApiState->overlayService = overlayService;
}

void SkyrimPlatform::AddTickTask(const std::function<void(Napi::Env)>& f)
{
  pImpl->tickTasks.AddTask(f);
}

void SkyrimPlatform::AddUpdateTask(const std::function<void(Napi::Env)>& f)
{
  pImpl->updateTasks.AddTask(f);
}

void SkyrimPlatform::PushAndWait(const std::function<void(Napi::Env)>& f)
{
  pImpl->pool.PushAndWait([this, f] {
    auto engine = pImpl->commonExecutionListener->GetJsEngine();
    engine->AcquireEnvAndCall(f);
  });
}

void SkyrimPlatform::Push(const std::function<void(Napi::Env)>& f)
{
  pImpl->pool.Push([this, f] {
    auto engine = pImpl->commonExecutionListener->GetJsEngine();
    engine->AcquireEnvAndCall(f);
  });
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
