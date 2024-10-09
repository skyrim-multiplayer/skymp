#include "BrowserApi.h"
#include "NullPointerException.h"

namespace {
thread_local bool g_cursorIsOpenByFocus = false;

static std::shared_ptr<BrowserApi::State> g_browserApiState;

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

Napi::Value BrowserApi::SetVisible(const Napi::CallbackInfo& info)
{
  bool& v = CEFUtils::DX11RenderHandler::Visible();
  v = NapiHelper::ExtractBoolean(info[0], "visible");
  return info.Env().Undefined();
}

Napi::Value BrowserApi::IsVisible(const Napi::CallbackInfo& info)
{
  return Napi::Boolean::New(info.Env(), CEFUtils::DX11RenderHandler::Visible());
}

Napi::Value BrowserApi::SetFocused(const Napi::CallbackInfo& info)
{
  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false)
    throw std::runtime_error("Chromium is disabled!");

  bool& v = CEFUtils::DInputHook::ChromeFocus();
  bool newFocus = NapiHelper::ExtractBoolean(info[0], "focused");
  if (v != newFocus) {
    v = newFocus;

    auto ui = RE::UI::GetSingleton();
    auto msgQ = RE::UIMessageQueue::GetSingleton();

    if (!ui || !msgQ)
      return info.Env().Undefined();

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
  return info.Env().Undefined();
}

Napi::Value BrowserApi::IsFocused(const Napi::CallbackInfo& info)
{
  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false)
    throw std::runtime_error("Chromium is disabled!");

  return Napi::Boolean::New(info.Env(), CEFUtils::DInputHook::ChromeFocus());
}

Napi::Value BrowserApi::LoadUrl(const Napi::CallbackInfo& info)
{
  const std::shared_ptr<State> &state = g_browserApiState;
  if (!state) {
    throw NullPointerException("state");
  }

  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false) {
    throw std::runtime_error("Chromium is disabled!");
  }

  auto str = NapiHelper::ExtractString(info[0], "url");
  return Napi::Boolean::New(info.Env(), GetApp(state).LoadUrl(str.data()));
}

Napi::Value BrowserApi::GetToken(const Napi::CallbackInfo& info)
{
  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false) {
    throw std::runtime_error("Chromium is disabled!");
  }

  return Napi::String::New(info.Env(), MyChromiumApp::GetCurrentSpToken());
}

Napi::Value BrowserApi::ExecuteJavaScript(const Napi::CallbackInfo& info)
{
  const std::shared_ptr<State> &state = g_browserApiState;
  if (!state) {
    throw NullPointerException("state");
  }

  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false) {
    throw std::runtime_error("Chromium is disabled!");
  }

  auto str = NapiHelper::ExtractString(info[0], "src");
  GetApp(state).ExecuteJavaScript(str);
  return info.Env().Undefined();
}

void BrowserApi::Register(Napi::Env env, Napi::Object& exports, std::shared_ptr<State> state)
{
  g_browserApiState = state;

  auto browser = Napi::Object::New(env);
  browser.Set("setVisible", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetVisible)));
  browser.Set("isVisible", Napi::Function::New(env, NapiHelper::WrapCppExceptions(IsVisible)));
  browser.Set("setFocused", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetFocused)));
  browser.Set("isFocused", Napi::Function::New(env, NapiHelper::WrapCppExceptions(IsFocused)));
  browser.Set("loadUrl", Napi::Function::New(env, NapiHelper::WrapCppExceptions(LoadUrl)));
  browser.Set("executeJavaScript", Napi::Function::New(env, NapiHelper::WrapCppExceptions(ExecuteJavaScript)));
  exports.Set("browser", browser);
}
