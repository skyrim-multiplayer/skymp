#include "BrowserApi.h"
#include "NullPointerException.h"
#include <cef/hooks/DInputHook.hpp>
#include <cef/ui/DX11RenderHandler.hpp>
#include <cef/ui/MyChromiumApp.hpp>
#include <skse64/GameMenus.h>

namespace {
thread_local bool g_cursorIsOpenByFocus = false;
}

JsValue BrowserApi::SetVisible(const JsFunctionArguments& args)
{
  bool& v = CEFUtils::DX11RenderHandler::Visible();
  v = (bool)args[1];
  return JsValue::Undefined();
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

JsValue BrowserApi::LoadUrl(const JsFunctionArguments& args,
                            std::shared_ptr<State> state)
{
  if (!state)
    throw NullPointerException("state");
  if (!state->overlayService)
    throw NullPointerException("MyChromiumApp");
  auto app = state->overlayService->GetMyChromiumApp();
  if (!app)
    throw NullPointerException("app");

  auto str = (std::string)args[1];
  return JsValue::Bool(app->LoadUrl(str.data()));
}

JsValue BrowserApi::GetToken(const JsFunctionArguments& args)
{
  return MyChromiumApp::GetCurrentSpToken();
}