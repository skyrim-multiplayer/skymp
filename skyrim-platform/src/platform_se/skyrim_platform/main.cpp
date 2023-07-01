#include "CallNativeApi.h"
#include "ConsoleApi.h"
#include "DumpFunctions.h"
#include "EventHandler.h"
#include "EventManager.h"
#include "EventsApi.h"
#include "FlowManager.h"
#include "FridaHooks.h"
#include "Hooks.h"
#include "IPC.h"
#include "InputConverter.h"
#include "PapyrusTESModPlatform.h"
#include "Settings.h"
#include "SkyrimPlatform.h"
#include "TPOverlayService.h"
#include "TPRenderSystemD3D11.h"
#include "TextsCollection.h"
#include "TickHandler.h"

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

void GetTextsToDraw(TextToDrawCallback callback)
{
  auto text = &TextsCollection::GetSingleton();

  for (const auto& a : TextsCollection::GetSingleton().GetCreatedTexts()) {
    callback(a.second);
  }
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

void OnUpdate(IVM* vm, StackID stackId)
{
  UpdateDumpFunctions();

  g_nativeCallRequirements.stackId = stackId;
  g_nativeCallRequirements.vm = vm;
  SkyrimPlatform::GetSingleton()->PrepareWorker();
  SkyrimPlatform::GetSingleton()->Push([=] {
    SkyrimPlatform::GetSingleton()->JsTick(true);
    SkyrimPlatform::GetSingleton()->StopWorker();
  });
  SkyrimPlatform::GetSingleton()->StartWorker();
  g_nativeCallRequirements.gameThrQ->Update();
  g_nativeCallRequirements.stackId = std::numeric_limits<StackID>::max();
  g_nativeCallRequirements.vm = nullptr;
}

void InitLog()
{
  auto path = logger::log_directory();
  if (!path) {
    stl::report_and_fail("Failed to find standard logging directory"sv);
  }

  *path /= "skyrim-platform.log"sv;
  auto sink =
    std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

  auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

  auto settings = Settings::GetPlatformSettings();
  auto logLevel =
    settings->GetInteger("Debug", "LogLevel", spdlog::level::level_enum::info);

  log->set_level(logLevel);
  log->flush_on(logLevel);

  spdlog::set_default_logger(std::move(log));
  spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

  logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

void InitCmd()
{
  auto settings = Settings::GetPlatformSettings();
  bool isCmd = settings->GetBool("Debug", "CMD", false);

  if (!isCmd) {
    return;
  }

  int offsetLeft = settings->GetInteger("Debug", "CmdOffsetLeft", 0);
  int offsetTop = settings->GetInteger("Debug", "CmdOffsetTop", 720);
  int width = settings->GetInteger("Debug", "CmdWidth", 1900);
  int height = settings->GetInteger("Debug", "CmdHeight", 317);
  bool isAlwaysOnTop = settings->GetBool("Debug", "CmdIsAlwaysOnTop", false);

  ConsoleApi::InitCmd(offsetLeft, offsetTop, width, height, isAlwaysOnTop);
}

extern "C" {
DLLEXPORT uint32_t SkyrimPlatform_IpcSubscribe_Impl(
  const char* systemName, IPC::MessageCallback callback, void* state)
{
  return IPC::Subscribe(systemName, callback, state);
}

DLLEXPORT void SkyrimPlatform_IpcUnsubscribe_Impl(uint32_t subscriptionId)
{
  return IPC::Unsubscribe(subscriptionId);
}

DLLEXPORT void SkyrimPlatform_IpcSend_Impl(const char* systemName,
                                           const uint8_t* data,
                                           uint32_t length)
{
  return IPC::Send(systemName, data, length);
}

DLLEXPORT bool SKSEAPI SKSEPlugin_Load_Impl(const SKSE::LoadInterface* skse)
{
  InitLog();

  InitCmd();

  logger::info("Loading plugin.");

  SKSE::Init(skse);
  SKSE::AllocTrampoline(64);

  const auto papyrusInterface = SKSE::GetPapyrusInterface();
  if (!papyrusInterface) {
    logger::critical("QueryInterface failed for PapyrusInterface");
    return false;
  }

  papyrusInterface->Register(TESModPlatform::Register);

  const auto messagingInterface = SKSE::GetMessagingInterface();
  if (!messagingInterface) {
    logger::critical("QueryInterface failed for MessagingInterface");
    return false;
  }

  messagingInterface->RegisterListener(EventHandler::HandleSKSEMessage);

  Hooks::Install();
  Frida::InstallHooks();

  // init custom events first
  // and the rest at DataLoaded, to be safe
  EventManager::InitCustom();

  TickHandler::GetSingleton()->Update();

  TESModPlatform::onPapyrusUpdate = OnUpdate;

  return true;
}
};

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
    pCursorX = &RE::MenuScreenData::GetSingleton()->mousePos.x;
    pCursorY = &RE::MenuScreenData::GetSingleton()->mousePos.y;
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
    auto ui = RE::UI::GetSingleton();
    if (!ui)
      return;

    if (!ui->IsMenuOpen(RE::CursorMenu::MENU_NAME))
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
    auto ui = RE::UI::GetSingleton();
    if (!ui)
      return;

    if (!ui->IsMenuOpen(RE::CursorMenu::MENU_NAME)) {
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
    REL::Relocation<void*> winMain{ Offsets::WinMain };
    return winMain.get();
  }

  bool Attach() override { return true; }

  bool Detach() override
  {
    FlowManager::CloseProcess(L"SkyrimSE.exe");
    FlowManager::CloseProcess(L"SkyrimPlatformCEF.exe.hidden");
    return true;
  }

  bool BeginMain() override
  {
    inputConverter = std::make_shared<InputConverter>();
    myInputListener = std::make_shared<MyInputListener>();

    CEFUtils::D3D11Hook::Install();
    CEFUtils::DInputHook::Install(myInputListener);
    CEFUtils::WindowsHook::Install();

    CEFUtils::DInputHook::Get().SetToggleKeys({ VK_F6 });
    CEFUtils::DInputHook::Get().SetEnabled(true);

    class ProcessMessageListenerImpl : public ProcessMessageListener
    {
    public:
      void OnProcessMessage(
        const std::string& name,
        const CefRefPtr<CefListValue>& arguments_) noexcept override
      {
        try {
          HandleMessage(name, arguments_);
        } catch (const std::exception&) {
          auto exception = std::current_exception();
          SkyrimPlatform::GetSingleton()->AddTickTask(
            [exception = std::move(exception)] {
              std::rethrow_exception(exception);
            });
        }
      }

    private:
      void HandleMessage(const std::string& name,
                         const CefRefPtr<CefListValue>& arguments_)
      {
        auto arguments = arguments_->Copy();
        SkyrimPlatform::GetSingleton()->AddTickTask([name, arguments] {
          auto length = static_cast<uint32_t>(arguments->GetSize());
          auto argumentsArray = JsValue::Array(length);
          for (uint32_t i = 0; i < length; ++i) {
            argumentsArray.SetProperty(
              static_cast<int>(i), CefValueToJsValue(arguments->GetValue(i)));
          }

          auto browserMessageEvent = JsValue::Object();
          browserMessageEvent.SetProperty("arguments", argumentsArray);
          EventsApi::SendEvent("browserMessage",
                               { JsValue::Undefined(), browserMessageEvent });
        });
      }

      static JsValue CefValueToJsValue(const CefRefPtr<CefValue>& cefValue)
      {
        switch (cefValue->GetType()) {
          case VTYPE_NULL:
            return JsValue::Null();
          case VTYPE_BOOL:
            return JsValue::Bool(cefValue->GetBool());
          case VTYPE_INT:
            return JsValue::Int(cefValue->GetInt());
          case VTYPE_DOUBLE:
            return JsValue::Double(cefValue->GetDouble());
          case VTYPE_STRING:
            return JsValue::String(cefValue->GetString().ToString());
          case VTYPE_DICTIONARY: {
            auto dict = cefValue->GetDictionary();
            auto result = JsValue::Object();
            CefDictionaryValue::KeyList keyList;
            dict->GetKeys(keyList);
            for (const std::string& key : keyList) {
              auto cefValue = dict->GetValue(key);
              auto jsValue = CefValueToJsValue(cefValue);
              result.SetProperty(key, jsValue);
            }
            return result;
          }
          case VTYPE_LIST: {
            auto list = cefValue->GetList();
            auto length = static_cast<int>(list->GetSize());
            auto result = JsValue::Array(length);
            for (int i = 0; i < length; ++i) {
              auto cefValue = list->GetValue(i);
              auto jsValue = CefValueToJsValue(cefValue);
              result.SetProperty(i, jsValue);
            }
            return result;
          }
          case VTYPE_BINARY:
          case VTYPE_INVALID:
            return JsValue::Undefined();
        }
        return JsValue::Undefined();
      }
    };

    auto onProcessMessage = std::make_shared<ProcessMessageListenerImpl>();

    ObtainTextsToDrawFunction obtainTextsToDraw = GetTextsToDraw;

    overlayService =
      std::make_shared<OverlayService>(onProcessMessage, obtainTextsToDraw);
    myInputListener->Init(overlayService, inputConverter);
    SkyrimPlatform::GetSingleton()->SetOverlayService(overlayService);
    renderSystem = std::make_shared<RenderSystemD3D11>(*overlayService);

    auto manager = RE::BSRenderManager::GetSingleton();
    if (!manager) {
      logger::critical("Failed to retrieve BSRenderManager");
    }

    renderSystem->m_pSwapChain =
      reinterpret_cast<IDXGISwapChain*>(manager->swapChain);

    return true;
  }

  bool EndMain() override
  {
    renderSystem.reset();
    overlayService.reset();
    return true;
  }

  void Update() override {}

  std::shared_ptr<OverlayService> overlayService;
  std::shared_ptr<RenderSystemD3D11> renderSystem;
  std::shared_ptr<MyInputListener> myInputListener;
  std::shared_ptr<InputConverter> inputConverter;
};

DEFINE_DLL_ENTRY_INITIALIZER(SkyrimPlatformApp);
