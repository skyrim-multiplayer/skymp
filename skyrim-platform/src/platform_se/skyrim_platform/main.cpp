#include "CameraApi.h"
#include "ConsoleApi.h"
#include "DevApi.h"
#include "DirectoryMonitor.h"
#include "DumpFunctions.h"
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
#include "PapyrusTESModPlatform.h"
#include "SkyrimPlatform.h"
#include "SkyrimPlatformProxy.h"
#include "TPInputService.h"
#include "TPOverlayService.h"
#include "TPRenderSystemD3D11.h"
#include "TaskQueue.h"
#include "ThreadPoolWrapper.h"
#include "TickTask.h"
#include "ui/TextToDraw.h"
#include <RE/ConsoleLog.h>
#include <Windows.h>
#include <atomic>
#include <functional>
#include <hooks/D3D11Hook.hpp>
#include <hooks/DInputHook.hpp>
#include <hooks/IInputListener.h>
#include <hooks/WindowsHook.hpp>
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

#define PLUGIN_NAME "SkyrimPlatform"
#define PLUGIN_VERSION 0

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

void SetupFridaHooks();

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

void OnTick()
{
  SkyrimPlatform::GetSingleton().PushAndWait(
    [=](int) { SkyrimPlatform::GetSingleton().JsTick(false); });
  TESModPlatform::Update();
}

void OnUpdate(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId)
{
  UpdateDumpFunctions();

  g_nativeCallRequirements.stackId = stackId;
  g_nativeCallRequirements.vm = vm;
  SkyrimPlatform::GetSingleton().PushAndWait(
    [=](int) { SkyrimPlatform::GetSingleton().JsTick(true); });
  g_nativeCallRequirements.gameThrQ->Update();
  g_nativeCallRequirements.stackId = std::numeric_limits<RE::VMStackID>::max();
  g_nativeCallRequirements.vm = nullptr;
}

std::vector<TextToDraw> GetTextsToDraw()
{
  std::vector<TextToDraw> textsToDraw = { TextToDraw(), TextToDraw(), TextToDraw()};
  return textsToDraw;
}

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
  auto taskInterface = reinterpret_cast<SKSETaskInterface*>(
    skse->QueryInterface(kInterface_Task));
  if (!taskInterface) {
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

  TickTask::Launch(taskInterface, OnTick);

  papyrusInterface->Register(
    (SKSEPapyrusInterface::RegisterFunctions)TESModPlatform::Register);
  TESModPlatform::onPapyrusUpdate = OnUpdate;

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
          SkyrimPlatform::GetSingleton().AddTickTask(
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
        SkyrimPlatform::GetSingleton().AddTickTask([name, arguments] {
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
            return JsValue::String(cefValue->GetString());
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

    std::function<std::vector<TextToDraw>()> ObtainTextsToDraw =
      GetTextsToDraw;
    overlayService =
      std::make_shared<OverlayService>(onProcessMessage, ObtainTextsToDraw);
    myInputListener->Init(overlayService, inputConverter);
    SkyrimPlatform::GetSingleton().SetOverlayService(overlayService);

    renderSystem = std::make_shared<RenderSystemD3D11>(*overlayService);
    renderSystem->m_pSwapChain = reinterpret_cast<IDXGISwapChain*>(
      BSRenderManager::GetSingleton()->swapChain);

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
