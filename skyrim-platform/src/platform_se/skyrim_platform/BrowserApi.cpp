#include "BrowserApi.h"
#include "BrowserApiNirnLab.h"
#include "BrowserApiTilted.h"

void BrowserApi::Register(Napi::Env env, Napi::Object& exports)
{
  logger::info("register");

  auto settings = Settings::GetPlatformSettings();
  std::string backendName =
    settings->GetString("Browser", "BackendName", "auto");

  auto browser = Napi::Object::New(env);
  if (backendName == "auto" || backendName == "tilted") {
    logger::info("using Tilted UI (legacy) backend for browser");
    browser.Set(
      "getBackend",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo& info) {
          auto result = Napi::Object::New(info.Env());
          result.Set("name", Napi::String::New(info.Env(), "tilted"));
          return result;
        })));
    browser.Set(
      "setVisible",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions(BrowserApiTilted::SetVisible)));
    browser.Set(
      "isVisible",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions(BrowserApiTilted::IsVisible)));
    browser.Set(
      "setFocused",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions(BrowserApiTilted::SetFocused)));
    browser.Set(
      "isFocused",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions(BrowserApiTilted::IsFocused)));
    browser.Set(
      "loadUrl",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions(BrowserApiTilted::LoadUrl)));
    browser.Set("executeJavaScript",
                Napi::Function::New(env,
                                    NapiHelper::WrapCppExceptions(
                                      BrowserApiTilted::ExecuteJavaScript)));
  } else if (backendName == "nirnlab") {
    logger::info("using NirnLab UI Platform backend for browser");
    browser.Set(
      "getBackend",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo& info) {
          auto result = Napi::Object::New(info.Env());
          result.Set("name", Napi::String::New(info.Env(), "nirnlab"));
          return result;
        })));
    browser.Set(
      "setVisible",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo& info) {
          return BrowserApiNirnLab::GetInstance().SetVisible(info);
        })));
    browser.Set(
      "isVisible",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo& info) {
          return BrowserApiNirnLab::GetInstance().IsVisible(info);
        })));
    browser.Set(
      "setFocused",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo& info) {
          return BrowserApiNirnLab::GetInstance().SetFocused(info);
        })));
    browser.Set(
      "isFocused",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo& info) {
          return BrowserApiNirnLab::GetInstance().IsFocused(info);
        })));
    browser.Set(
      "loadUrl",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo& info) {
          return BrowserApiNirnLab::GetInstance().LoadUrl(info);
        })));
    browser.Set(
      "executeJavaScript",
      Napi::Function::New(
        env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo& info) {
          return BrowserApiNirnLab::GetInstance().ExecuteJavaScript(info);
        })));
  } else {
    throw std::runtime_error("Bad BackendName in SkyrimPlatform.ini: '" +
                             backendName +
                             "'. Must be one of auto/tilted/nirnlab");
  }
  exports.Set("browser", browser);
}
