#include "BrowserApiNirnLab.h"

#include <NirnLabUIPlatformAPI/API.h>
#include <NirnLabUIPlatformAPI/SKSELoader.h>

#include "EventsApi.h"
#include "NapiHelper.h"
#include "SkyrimPlatform.h"

void BrowserApiNirnLab::HandleSkseMessage(
  SKSE::MessagingInterface::Message* a_msg)
{
  logger::info("skse message type {}", a_msg->type);
  NL::UI::Settings settings;
  settings.remoteDebuggingPort = 9000;
  NL::UI::SKSELoader::ProcessSKSEMessage(a_msg, &settings);
}

BrowserApiNirnLab::BrowserApiNirnLab()
{
  NL::UI::SKSELoader::GetUIPlatformAPIWithVersionCheck(
    [](NL::UI::IUIPlatformAPI* receivedApi) {
      auto& self = GetInstance();
      self.api = receivedApi;
      self.ApiInit();
    });
}

BrowserApiNirnLab& BrowserApiNirnLab::GetInstance()
{
  static BrowserApiNirnLab g_inst;
  return g_inst;
}

Napi::Value BrowserApiNirnLab::SetVisible(const Napi::CallbackInfo& info)
{
  wantedIsVisible = NapiHelper::ExtractBoolean(info[0], "isVisible");
  logger::info("SetVisible {}", wantedIsVisible);
  UpdateVisible();
  return info.Env().Undefined();
}

Napi::Value BrowserApiNirnLab::IsVisible(const Napi::CallbackInfo& info)
{
  return Napi::Boolean::New(info.Env(), wantedIsVisible);
}

bool BrowserApiNirnLab::IsVisible()
{
  return wantedIsVisible;
}

Napi::Value BrowserApiNirnLab::SetFocused(const Napi::CallbackInfo& info)
{
  wantedIsFocused = NapiHelper::ExtractBoolean(info[0], "isFocused");
  logger::info("SetFocused {}", wantedIsFocused);
  UpdateFocused();
  return info.Env().Undefined();
}

Napi::Value BrowserApiNirnLab::IsFocused(const Napi::CallbackInfo& info)
{
  return Napi::Boolean::New(info.Env(), wantedIsFocused);
}

Napi::Value BrowserApiNirnLab::LoadUrl(const Napi::CallbackInfo& info)
{
  wantedUrl = NapiHelper::ExtractString(info[0], "url");
  logger::info("LoadUrl {}", wantedUrl);
  UpdateUrl();
  return Napi::Boolean::New(info.Env(), true);
}

Napi::Value BrowserApiNirnLab::GetToken(const Napi::CallbackInfo& info)
{
  return Napi::String::New(info.Env(), "deprecated");
}

Napi::Value BrowserApiNirnLab::ExecuteJavaScript(
  const Napi::CallbackInfo& info)
{
  auto src = NapiHelper::ExtractString(info[0], "src");
  {
    auto d = src.substr(0, 120);
    for (char& c : d) {
      if (c == '\n') {
        c = ' ';
      }
    }
    logger::info("JS {} ...", d);
  }
  jsExecQueue.push_back(src);
  UpdateJs();
  return info.Env().Undefined();
}

void BrowserApiNirnLab::UpdateVisible()
{
  if (!browser) {
    return;
  }
  browser->SetBrowserVisible(wantedIsVisible);
}

void BrowserApiNirnLab::UpdateFocused()
{
  if (!browser) {
    return;
  }
  browser->SetBrowserFocused(wantedIsFocused);
}

void BrowserApiNirnLab::UpdateUrl()
{
  if (!browser) {
    return;
  }
  browser->LoadBrowserURL(wantedUrl.c_str(), false);
}

void BrowserApiNirnLab::UpdateJs()
{
  if (!browser) {
    return;
  }
  while (!jsExecQueue.empty()) {
    browser->ExecuteJavaScript(jsExecQueue.front().c_str());
    jsExecQueue.pop_front();
  }
}

void BrowserApiNirnLab::UpdateAll()
{
  UpdateVisible();
  UpdateFocused();
  UpdateUrl();
  UpdateJs();
}

void BrowserApiNirnLab::ApiInit()
{
  if (api == nullptr) {
    throw std::runtime_error(
      "BrowserApiNirnLab::ApiInit: api must not be null here");
  }

  NL::JS::JSFuncInfo callback{
    .objectName = "skyrimPlatform",
    .funcName = "sendMessage",
    .callbackData = {
      .callback = [](const char** a_args, int a_argsCount) {
        std::vector<std::string> args{a_args, a_args + a_argsCount};
        SkyrimPlatform::GetSingleton()->AddTickTask(
          [args = std::move(args)](Napi::Env env) {
            auto argumentsArray = Napi::Array::New(env, args.size());
            for (uint32_t i = 0; i < args.size(); ++i) {
              argumentsArray.Set(i, NapiHelper::ParseJson(env, args[i]));
            }

            auto browserMessageEvent = Napi::Object::New(env);
            browserMessageEvent.Set("arguments", argumentsArray);
            EventsApi::SendEvent("browserMessage", { browserMessageEvent });
          });
      },
      .executeInGameThread = true,
      .isEventFunction = false,
    },
  };
  auto callbackPtr = &callback;

  constexpr auto kNirnlabBrowserName = "SkyrimPlatform_Default";

  const NL::UI::IUIPlatformAPI::BrowserRefHandle browserHandle =
    api->AddOrGetBrowser(kNirnlabBrowserName, &callbackPtr, 1,
                         "file:///Data/Platform/UI/index.html", browser);
  if (browserHandle == NL::UI::IUIPlatformAPI::InvalidBrowserRefHandle) {
    logger::error("browser init failed: InvalidBrowserRefHandle");
    return;
  }
  if (!browser) {
    logger::error("browser init failed: browser is nullptr");
    return;
  }

  UpdateAll();
}
