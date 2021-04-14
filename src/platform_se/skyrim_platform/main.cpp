#include "BrowserApi.h"
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
#include "InputConverter.h"
#include "InventoryApi.h"
#include "JsEngine.h"
#include "LoadGameApi.h"
#include "MpClientPluginApi.h"
#include "MyUpdateTask.h"
#include "PapyrusTESModPlatform.h"
#include "ReadFile.h"
#include "SkyrimPlatformProxy.h"
#include "SystemPolyfill.h"
#include "TPInputService.h"
#include "TPOverlayService.h"
#include "TPRenderSystemD3D11.h"
#include "TaskQueue.h"
#include "ThreadPoolWrapper.h"
#include <RE/ConsoleLog.h>
#include <SKSE/API.h>
#include <SKSE/Interfaces.h>
#include <SKSE/Stubs.h>
#include <Windows.h>
#include <atomic>
#include <cef/hooks/D3D11Hook.hpp>
#include <cef/hooks/DInputHook.hpp>
#include <cef/hooks/IInputListener.h>
#include <cef/hooks/WindowsHook.hpp>
#include <cef/reverse/App.hpp>
#include <cef/reverse/AutoPtr.hpp>
#include <cef/reverse/Entry.hpp>
#include <cef/ui/MyChromiumApp.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <shlobj.h>
#include <skse64/GameMenus.h>
#include <skse64/GameReferences.h>
#include <skse64/NiRenderer.h>
#include <skse64/PluginAPI.h>
#include <skse64/gamethreads.h>
#include <sstream>
#include <string>
#include <thread>

#define PLUGIN_NAME "SkyrimPlatform"
#define PLUGIN_VERSION 0

void StartSKSE(void* hDllHandle);
void SetupFridaHooks();

static SKSETaskInterface* g_taskInterface = nullptr;
static SKSEMessagingInterface* g_messaging = nullptr;
ThreadPoolWrapper g_pool;
HttpClient g_httpClient;
std::shared_ptr<BrowserApi::State> g_browserApiState(new BrowserApi::State);

CallNativeApi::NativeCallRequirements g_nativeCallRequirements;
TaskQueue g_taskQueue;

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
        engine->ResetContext(g_taskQueue);
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
                BrowserApi::Register(e, g_browserApiState);
                InventoryApi::Register(e);
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
__declspec(dllexport) bool SKSEPlugin_Query_Impl(
  const SKSE::QueryInterface* skse, SKSE::PluginInfo* info)
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

__declspec(dllexport) bool SKSEPlugin_Load_Impl(const SKSEInterface* skse)
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

#define POINTER_SKYRIMSE(className, variableName, ...)                        \
  static CEFUtils::AutoPtr<className> variableName(__VA_ARGS__)

inline uint32_t GetCefModifiers_(uint16_t aVirtualKey)
{
  uint32_t modifiers = EVENTFLAG_NONE;

  if (GetAsyncKeyState(VK_MENU) & 0x8000) {
    modifiers |= EVENTFLAG_ALT_DOWN;
  }

  if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
    modifiers |= EVENTFLAG_CONTROL_DOWN;
  }

  if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
    modifiers |= EVENTFLAG_SHIFT_DOWN;
  }

  if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
    modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
  }

  if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
    modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
  }

  if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
    modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
  }

  if (GetAsyncKeyState(VK_CAPITAL) & 1) {
    modifiers |= EVENTFLAG_CAPS_LOCK_ON;
  }

  if (GetAsyncKeyState(VK_NUMLOCK) & 1) {
    modifiers |= EVENTFLAG_NUM_LOCK_ON;
  }

  if (aVirtualKey) {
    if (aVirtualKey == VK_RCONTROL || aVirtualKey == VK_RMENU ||
        aVirtualKey == VK_RSHIFT) {
      modifiers |= EVENTFLAG_IS_RIGHT;
    } else if (aVirtualKey == VK_LCONTROL || aVirtualKey == VK_LMENU ||
               aVirtualKey == VK_LSHIFT) {
      modifiers |= EVENTFLAG_IS_LEFT;
    } else if (aVirtualKey >= VK_NUMPAD0 && aVirtualKey <= VK_DIVIDE) {
      modifiers |= EVENTFLAG_IS_KEY_PAD;
    }
  }

  return modifiers;
}

class MyInputListener : public IInputListener
{
public:
  bool IsBrowserFocused() { return CEFUtils::DInputHook::ChromeFocus(); }

  MyInputListener()
  {
    pCursorX = (float*)(REL::Module::BaseAddr() + 0x2F6C104);
    pCursorY = (float*)(REL::Module::BaseAddr() + 0x2F6C108);
    vkCodeDownDur.fill(0);
  }

  void Init(std::shared_ptr<OverlayService> service_,
            std::shared_ptr<InputConverter> conv_)
  {
    service = service_;
    conv = conv_;
  }

  void InjectChar(uint8_t code)
  {
    if (auto app = service->GetMyChromiumApp()) {
      int virtualKeyCode = VscToVk(code);
      int scan = code;
      auto capitalizeLetters = GetCefModifiers_(virtualKeyCode) &
        (EVENTFLAG_SHIFT_DOWN | EVENTFLAG_CAPS_LOCK_ON);
      auto ch = conv->VkCodeToChar(virtualKeyCode, capitalizeLetters);
      if (ch)
        app->InjectKey(cef_key_event_type_t::KEYEVENT_CHAR,
                       GetCefModifiers_(virtualKeyCode), ch, scan);
    }
  }

  void InjectKey(uint8_t code, bool down)
  {
    if (auto app = service->GetMyChromiumApp()) {
      int virtualKeyCode = VscToVk(code);
      int scan = code;
      app->InjectKey(down ? cef_key_event_type_t::KEYEVENT_KEYDOWN
                          : cef_key_event_type_t::KEYEVENT_KEYUP,
                     GetCefModifiers_(virtualKeyCode), virtualKeyCode, scan);
    }
  }

  int VscToVk(int code)
  {
    int vk = MapVirtualKeyA(code, MAPVK_VSC_TO_VK);
    if (code == 203)
      return VK_LEFT;
    if (code == 205)
      return VK_RIGHT;
    return vk;
  }

  void OnKeyStateChange(uint8_t code, bool down) noexcept override
  {
    if (!IsBrowserFocused())
      return;

    // Switch layout if need
    bool switchLayoutDown = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
                             (GetAsyncKeyState(VK_MENU) & 0x8000)) ||
      (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
        (GetAsyncKeyState(VK_CONTROL) & 0x8000);
    if (switchLayoutDownWas != switchLayoutDown) {
      switchLayoutDownWas = switchLayoutDown;
      if (switchLayoutDown) {
        conv->SwitchLayout();
      }
    }

    // Fill vkCodeDownDur
    int virtualKeyCode = VscToVk(code);
    if (virtualKeyCode >= 0 && virtualKeyCode < vkCodeDownDur.size()) {
      vkCodeDownDur[virtualKeyCode] = down ? clock() : 0;
    }

    if (auto app = service->GetMyChromiumApp()) {
      InjectKey(code, down);

      if (down) {
        InjectChar(code);
      }
    }
  }

  void OnMouseWheel(int32_t delta) noexcept override
  {
    if (!IsBrowserFocused())
      return;
    if (pCursorX && pCursorY)
      if (auto app = service->GetMyChromiumApp()) {
        app->InjectMouseWheel(*pCursorX, *pCursorY, delta,
                              GetCefModifiers_(0));
      }
  }

  void OnMouseMove(float deltaX, float deltaY) noexcept override
  {
    auto mm = MenuManager::GetSingleton();
    if (!mm)
      return;
    static const auto fs = new BSFixedString("Cursor Menu");
    if (!mm->IsMenuOpen(fs))
      return;

    if (pCursorX && pCursorY)
      if (auto app = service->GetMyChromiumApp()) {
        app->InjectMouseMove(*pCursorX, *pCursorY, GetCefModifiers_(0),
                             IsBrowserFocused());
      }
  }

  void OnMouseStateChange(MouseButton mouseButton, bool down) noexcept override
  {
    if (!IsBrowserFocused())
      return;
    if (pCursorX && pCursorY)
      if (auto app = service->GetMyChromiumApp()) {
        cef_mouse_button_type_t btn;
        switch (mouseButton) {
          case MouseButton::Left:
            btn = cef_mouse_button_type_t::MBT_LEFT;
            break;
          case MouseButton::Middle:
            btn = cef_mouse_button_type_t::MBT_MIDDLE;
            break;
          case MouseButton::Right:
            btn = cef_mouse_button_type_t::MBT_RIGHT;
            break;
        }
        app->InjectMouseButton(*pCursorX, *pCursorY, btn, !down,
                               GetCefModifiers_(0));
      }
  }

  void OnUpdate() noexcept override
  {
    auto mm = MenuManager::GetSingleton();
    if (!mm)
      return;
    static const auto fs = new BSFixedString("Cursor Menu");
    if (!mm->IsMenuOpen(fs)) {
      if (auto app = service->GetMyChromiumApp()) {
        app->InjectMouseMove(-1.f, -1.f, GetCefModifiers_(0), false);
      }
    }
    if (auto app = service->GetMyChromiumApp())
      app->RunTasks();

    // Repeat the character until the key isn't released
    for (int i = 0; i < 256; ++i) {
      const auto pressMoment = this->vkCodeDownDur[i];
      if (pressMoment && clock() - pressMoment > CLOCKS_PER_SEC / 2) {
        if (i == VK_BACK || i == VK_RIGHT || i == VK_LEFT) {
          InjectKey(MapVirtualKeyA(i, MAPVK_VK_TO_VSC), true);
          InjectKey(MapVirtualKeyA(i, MAPVK_VK_TO_VSC), false);
        } else {
          InjectChar(MapVirtualKeyA(i, MAPVK_VK_TO_VSC));
        }
      }
    }
  }

private:
  std::shared_ptr<OverlayService> service;
  std::shared_ptr<InputConverter> conv;
  std::array<clock_t, 256> vkCodeDownDur;
  float* pCursorX = nullptr;
  float* pCursorY = nullptr;
  bool switchLayoutDownWas = false;
};

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
    inputConverter.reset(new InputConverter);

    myInputListener.reset(new MyInputListener);

    CEFUtils::D3D11Hook::Install();
    CEFUtils::DInputHook::Install(myInputListener);
    CEFUtils::WindowsHook::Install();

    CEFUtils::DInputHook::Get().SetToggleKeys({ VK_F6 });
    CEFUtils::DInputHook::Get().SetEnabled(true);

    overlayService.reset(new OverlayService);
    overlayService->GetMyChromiumApp();
    myInputListener->Init(overlayService, inputConverter);
    g_browserApiState->overlayService = overlayService;

    // inputService.reset(new InputService(*overlayService));
    renderSystem.reset(new RenderSystemD3D11(*overlayService));
    renderSystem->m_pSwapChain = reinterpret_cast<IDXGISwapChain*>(
      BSRenderManager::GetSingleton()->swapChain);

    return true;
  }

  bool EndMain() override
  {
    // inputService.reset();
    renderSystem.reset();
    overlayService.reset();
    return true;
  }

  void Update() override {}

  std::shared_ptr<OverlayService> overlayService;
  // std::shared_ptr<InputService> inputService;
  std::shared_ptr<RenderSystemD3D11> renderSystem;
  std::shared_ptr<MyInputListener> myInputListener;
  std::shared_ptr<InputConverter> inputConverter;
};

DEFINE_DLL_ENTRY_INITIALIZER(SkyrimPlatformApp);