#include "BrowserApi.h"
#include "BrowserApiNirnLab.h"
#include "BrowserApiTilted.h"

namespace {
void CheckIfChromiumEnabled()
{
  auto settings = Settings::GetPlatformSettings();
  if (settings->GetBool("Debug", "ChromiumEnabled", true) == false) {
    throw std::runtime_error("Chromium is disabled!");
  }
}
}

Napi::Value BrowserApi::GetBackend(const Napi::CallbackInfo& info)
{
  CheckIfChromiumEnabled();

  auto settings = Settings::GetPlatformSettings();
  std::string backendName =
    settings->GetString("Browser", "BackendName", "auto");

  if (backendName == "auto" || backendName == "tilted") {
    auto result = Napi::Object::New(info.Env());
    result.Set("name", Napi::String::New(info.Env(), "nirnlab"));
    return result;
  } else if (backendName == "nirnlab") {
    auto result = Napi::Object::New(info.Env());
    result.Set("name", Napi::String::New(info.Env(), "nirnlab"));
    return result;
  } else {
    throw std::runtime_error("Bad BackendName in SkyrimPlatform.ini: '" +
                             backendName +
                             "'. Must be one of auto/tilted/nirnlab");
  }
}

Napi::Value BrowserApi::SetVisible(const Napi::CallbackInfo& info)
{
  CheckIfChromiumEnabled();

  auto settings = Settings::GetPlatformSettings();
  std::string backendName =
    settings->GetString("Browser", "BackendName", "auto");

  if (backendName == "auto" || backendName == "tilted") {
    return BrowserApiTilted::SetVisible(info);
  } else if (backendName == "nirnlab") {
    return BrowserApiNirnLab::GetInstance().SetVisible(info);
  } else {
    throw std::runtime_error("Bad BackendName in SkyrimPlatform.ini: '" +
                             backendName +
                             "'. Must be one of auto/tilted/nirnlab");
  }
}

Napi::Value BrowserApi::IsVisible(const Napi::CallbackInfo& info)
{
  CheckIfChromiumEnabled();

  auto settings = Settings::GetPlatformSettings();
  std::string backendName =
    settings->GetString("Browser", "BackendName", "auto");

  if (backendName == "auto" || backendName == "tilted") {
    return BrowserApiTilted::IsVisible(info);
  } else if (backendName == "nirnlab") {
    return BrowserApiNirnLab::GetInstance().IsVisible(info);
  } else {
    throw std::runtime_error("Bad BackendName in SkyrimPlatform.ini: '" +
                             backendName +
                             "'. Must be one of auto/tilted/nirnlab");
  }
}

Napi::Value BrowserApi::SetFocused(const Napi::CallbackInfo& info)
{
  CheckIfChromiumEnabled();

  auto settings = Settings::GetPlatformSettings();
  std::string backendName =
    settings->GetString("Browser", "BackendName", "auto");

  if (backendName == "auto" || backendName == "tilted") {
    return BrowserApiTilted::SetFocused(info);
  } else if (backendName == "nirnlab") {
    return BrowserApiNirnLab::GetInstance().SetFocused(info);
  } else {
    throw std::runtime_error("Bad BackendName in SkyrimPlatform.ini: '" +
                             backendName +
                             "'. Must be one of auto/tilted/nirnlab");
  }
}

Napi::Value BrowserApi::IsFocused(const Napi::CallbackInfo& info)
{
  CheckIfChromiumEnabled();

  auto settings = Settings::GetPlatformSettings();
  std::string backendName =
    settings->GetString("Browser", "BackendName", "auto");

  if (backendName == "auto" || backendName == "tilted") {
    return BrowserApiTilted::IsFocused(info);
  } else if (backendName == "nirnlab") {
    return BrowserApiNirnLab::GetInstance().IsFocused(info);
  } else {
    throw std::runtime_error("Bad BackendName in SkyrimPlatform.ini: '" +
                             backendName +
                             "'. Must be one of auto/tilted/nirnlab");
  }
}

Napi::Value BrowserApi::LoadUrl(const Napi::CallbackInfo& info)
{
  CheckIfChromiumEnabled();

  auto settings = Settings::GetPlatformSettings();
  std::string backendName =
    settings->GetString("Browser", "BackendName", "auto");

  if (backendName == "auto" || backendName == "tilted") {
    return BrowserApiTilted::LoadUrl(info);
  } else if (backendName == "nirnlab") {
    return BrowserApiNirnLab::GetInstance().LoadUrl(info);
  } else {
    throw std::runtime_error("Bad BackendName in SkyrimPlatform.ini: '" +
                             backendName +
                             "'. Must be one of auto/tilted/nirnlab");
  }
}

Napi::Value BrowserApi::GetToken(const Napi::CallbackInfo& info)
{
  CheckIfChromiumEnabled();

  auto settings = Settings::GetPlatformSettings();
  std::string backendName =
    settings->GetString("Browser", "BackendName", "auto");

  if (backendName == "auto" || backendName == "tilted") {
    return BrowserApiTilted::GetToken(info);
  } else if (backendName == "nirnlab") {
    return BrowserApiNirnLab::GetInstance().GetToken(info);
  } else {
    throw std::runtime_error("Bad BackendName in SkyrimPlatform.ini: '" +
                             backendName +
                             "'. Must be one of auto/tilted/nirnlab");
  }
}

Napi::Value BrowserApi::ExecuteJavaScript(const Napi::CallbackInfo& info)
{
  CheckIfChromiumEnabled();

  auto settings = Settings::GetPlatformSettings();
  std::string backendName =
    settings->GetString("Browser", "BackendName", "auto");

  if (backendName == "auto" || backendName == "tilted") {
    return BrowserApiTilted::ExecuteJavaScript(info);
  } else if (backendName == "nirnlab") {
    return BrowserApiNirnLab::GetInstance().ExecuteJavaScript(info);
  } else {
    throw std::runtime_error("Bad BackendName in SkyrimPlatform.ini: '" +
                             backendName +
                             "'. Must be one of auto/tilted/nirnlab");
  }
}

void BrowserApi::Register(Napi::Env env, Napi::Object& exports)
{
  auto browser = Napi::Object::New(env);
  browser.Set(
    "getBackend",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetBackend)));
  browser.Set(
    "setVisible",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetVisible)));
  browser.Set(
    "isVisible",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(IsVisible)));
  browser.Set(
    "setFocused",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetFocused)));
  browser.Set(
    "isFocused",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(IsFocused)));
  browser.Set(
    "loadUrl",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(LoadUrl)));
  browser.Set("executeJavaScript",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(ExecuteJavaScript)));
  exports.Set("browser", browser);
}
