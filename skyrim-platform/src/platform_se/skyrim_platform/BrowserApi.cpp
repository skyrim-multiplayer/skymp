#include "BrowserApi.h"
#include "NullPointerException.h"
#include <hooks/DInputHook.hpp>
#include <skse64/GameMenus.h>
#include <ui/DX11RenderHandler.h>
#include <ui/MyChromiumApp.h>

namespace {
thread_local bool g_cursorIsOpenByFocus = false;

inline CEFUtils::MyChromiumApp& GetApp(
  const std::shared_ptr<BrowserApi::State>& state)
{
  if (!state)
    throw NullPointerException("state");
  if (!state->overlayService)
    throw NullPointerException("MyChromiumApp");
  auto app = state->overlayService->GetMyChromiumApp();
  if (!app)
    throw NullPointerException("app");
  return *app;
}
}

JsValue BrowserApi::SetVisible(const JsFunctionArguments& args)
{
  bool& v = CEFUtils::DX11RenderHandler::Visible();
  v = (bool)args[1];
  return JsValue::Undefined();
}

JsValue BrowserApi::IsVisible(const JsFunctionArguments& args)
{
  return JsValue::Bool(CEFUtils::DX11RenderHandler::Visible());
}

JsValue BrowserApi::SetFocused(const JsFunctionArguments& args)
{
  bool& v = CEFUtils::DInputHook::ChromeFocus();
  bool newFocus = (bool)args[1];
  if (v != newFocus) {
    v = newFocus;

    auto mm = MenuManager::GetSingleton();
    if (!mm)
      return JsValue::Undefined();

    static const auto fsCursorMenu = new BSFixedString("Cursor Menu");
    const bool alreadyOpen = mm->IsMenuOpen(fsCursorMenu);

    if (newFocus) {
      if (!alreadyOpen) {
        CALL_MEMBER_FN(UIManager::GetSingleton(), AddMessage)
        (fsCursorMenu, UIMessage::kMessage_Open, NULL);
        g_cursorIsOpenByFocus = true;
      }
    } else {
      if (g_cursorIsOpenByFocus) {
        CALL_MEMBER_FN(UIManager::GetSingleton(), AddMessage)
        (fsCursorMenu, UIMessage::kMessage_Close, NULL);
        g_cursorIsOpenByFocus = false;
      }
    }
  }
  return JsValue::Undefined();
}

JsValue BrowserApi::IsFocused(const JsFunctionArguments& args)
{
  return JsValue::Bool(CEFUtils::DInputHook::ChromeFocus());
}

JsValue BrowserApi::LoadUrl(const JsFunctionArguments& args,
                            std::shared_ptr<State> state)
{
  auto str = static_cast<std::string>(args[1]);
  return JsValue::Bool(GetApp(state).LoadUrl(str.data()));
}

JsValue BrowserApi::GetToken(const JsFunctionArguments& args)
{
  return MyChromiumApp::GetCurrentSpToken();
}

JsValue BrowserApi::ExecuteJavaScript(const JsFunctionArguments& args,
                                      std::shared_ptr<State> state)
{
  auto str = static_cast<std::string>(args[1]);
  GetApp(state).ExecuteJavaScript(str);
  return JsValue::Undefined();
}
