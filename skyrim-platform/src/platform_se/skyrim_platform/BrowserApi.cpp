#include "BrowserApi.h"
#include "NullPointerException.h"

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
  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false)
    throw std::runtime_error("Chromium is disabled!");

  bool& v = CEFUtils::DInputHook::ChromeFocus();
  bool newFocus = (bool)args[1];
  if (v != newFocus) {
    v = newFocus;

    auto ui = RE::UI::GetSingleton();
    auto msgQ = RE::UIMessageQueue::GetSingleton();

    if (!ui || !msgQ)
      return JsValue::Undefined();

    const bool alreadyOpen = ui->IsMenuOpen(RE::CursorMenu::MENU_NAME);

    if (newFocus) {
      if (!alreadyOpen) {
        msgQ->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow,
                         NULL);
        g_cursorIsOpenByFocus = true;
      }
    } else {
      if (g_cursorIsOpenByFocus) {
        msgQ->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide,
                         NULL);
        g_cursorIsOpenByFocus = false;
      }
    }
  }
  return JsValue::Undefined();
}

JsValue BrowserApi::IsFocused(const JsFunctionArguments& args)
{
  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false)
    throw std::runtime_error("Chromium is disabled!");

  return JsValue::Bool(CEFUtils::DInputHook::ChromeFocus());
}

JsValue BrowserApi::LoadUrl(const JsFunctionArguments& args,
                            std::shared_ptr<State> state)
{
  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false)
    throw std::runtime_error("Chromium is disabled!");

  auto str = static_cast<std::string>(args[1]);
  return JsValue::Bool(GetApp(state).LoadUrl(str.data()));
}

JsValue BrowserApi::GetToken(const JsFunctionArguments& args)
{
  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false)
    throw std::runtime_error("Chromium is disabled!");

  return MyChromiumApp::GetCurrentSpToken();
}

JsValue BrowserApi::ExecuteJavaScript(const JsFunctionArguments& args,
                                      std::shared_ptr<State> state)
{
  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false)
    throw std::runtime_error("Chromium is disabled!");

  auto str = static_cast<std::string>(args[1]);
  GetApp(state).ExecuteJavaScript(str);
  return JsValue::Undefined();
}
