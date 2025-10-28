#include "BrowserApiNirnLab.h"

#include <NirnLabUIPlatformAPI/API.h>

#include "EventsApi.h"
#include "NapiHelper.h"
#include "SkyrimPlatform.h"

void BrowserApiNirnLab::HandleSkseMessage(
  SKSE::MessagingInterface::Message* a_msg)
{
  logger::info("skse message type {}", a_msg->type);
  switch (a_msg->type) {
    case SKSE::MessagingInterface::kPostPostLoad: {
      SKSE::GetMessagingInterface()->RegisterListener(
        NL::UI::LibVersion::PROJECT_NAME, HandleNirnLabMessage);
      // All plugins are loaded. Request lib version.
      SKSE::GetMessagingInterface()->Dispatch(
        NL::UI::APIMessageType::RequestVersion, nullptr, 0,
        NL::UI::LibVersion::PROJECT_NAME);
    } break;
    case SKSE::MessagingInterface::kInputLoaded: {
      NL::UI::Settings settings;
      settings.remoteDebuggingPort = 9000;
      // API version is ok. Request interface.
      SKSE::GetMessagingInterface()->Dispatch(
        NL::UI::APIMessageType::RequestAPI, &settings, sizeof(settings),
        NL::UI::LibVersion::PROJECT_NAME);
    } break;
    default:
      break;
  }
}

void BrowserApiNirnLab::HandleNirnLabMessage(
  SKSE::MessagingInterface::Message* a_msg)
{
  auto& self = BrowserApiNirnLab::GetInstance();
  spdlog::info("Received message({}) from \"{}\"", a_msg->type,
               a_msg->sender ? a_msg->sender : "nullptr");
  switch (a_msg->type) {
    case NL::UI::APIMessageType::ResponseVersion: {
      const auto versionInfo =
        reinterpret_cast<NL::UI::ResponseVersionMessage*>(a_msg->data);
      spdlog::info(
        "NirnLabUIPlatform version: {}.{}",
        NL::UI::LibVersion::GetMajorVersion(versionInfo->libVersion),
        NL::UI::LibVersion::GetMinorVersion(versionInfo->libVersion));

      const auto majorAPIVersion =
        NL::UI::APIVersion::GetMajorVersion(versionInfo->apiVersion);
      // If the major version is different from ours, then using the API
      // may cause problems
      if (majorAPIVersion != NL::UI::APIVersion::MAJOR) {
        spdlog::error(
          "Can't use this API version of NirnLabUIPlatform. We have "
          "{}.{} and installed is {}.{}",
          NL::UI::APIVersion::MAJOR, NL::UI::APIVersion::MINOR,
          NL::UI::APIVersion::GetMajorVersion(versionInfo->apiVersion),
          NL::UI::APIVersion::GetMinorVersion(versionInfo->apiVersion));
      } else {
        self.api.versionChecked = true;
        spdlog::info(
          "API version is ok. We have {}.{} and installed is {}.{}",
          NL::UI::APIVersion::MAJOR, NL::UI::APIVersion::MINOR,
          NL::UI::APIVersion::GetMajorVersion(versionInfo->apiVersion),
          NL::UI::APIVersion::GetMinorVersion(versionInfo->apiVersion));
        self.ApiInit();
      }
      break;
    }
    case NL::UI::APIMessageType::ResponseAPI: {
      self.api.api =
        reinterpret_cast<NL::UI::ResponseAPIMessage*>(a_msg->data)->API;
      self.ApiInit();
      break;
    }
    default:
      break;
  }
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
  if (!api.Ready()) {
    return;
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
