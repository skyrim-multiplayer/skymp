#include "BrowserApiNirnLab.h"

#include "EventsApi.h"
#include "NapiHelper.h"
#include "SkyrimPlatform.h"

void BrowserApiNirnLab::Register(Napi::Env env, Napi::Object& exports)
{
  auto browser = Napi::Object::New(env);

  browser.Set(
    "getBackend",
    Napi::Function::New(
      env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo& info) {
        return BrowserApiNirnLab::GetInstance().GetBackend(info);
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
  exports.Set("browser", browser);
}

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
      NL::UI::Settings defaultSettings;
      // API version is ok. Request interface.
      SKSE::GetMessagingInterface()->Dispatch(
        NL::UI::APIMessageType::RequestAPI, &defaultSettings,
        sizeof(defaultSettings), NL::UI::LibVersion::PROJECT_NAME);
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
          "Can't using this API version of NirnLabUIPlatform. We have "
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

Napi::Value BrowserApiNirnLab::GetBackend(const Napi::CallbackInfo& info)
{
  auto result = Napi::Object::New(info.Env());
  result.Set("name", Napi::String::New(info.Env(), "nirnlab"));
  return result;
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

Napi::Value BrowserApiNirnLab ::SetFocused(const Napi::CallbackInfo& info)
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
  logger::info("UpdateFocused()");
  if (!browser) {
    return;
  }
  browser->SetBrowserFocused(wantedIsFocused);
  logger::info("UpdateFocused() done -> {}", wantedIsFocused);
}

void BrowserApiNirnLab::UpdateUrl()
{
  if (!browser) {
    return;
  }
}

void BrowserApiNirnLab ::UpdateJs()
{
  // logger::info("update js");
  if (!browser) {
    return;
  }
  while (!jsExecQueue.empty()) {
    // logger::info("real exec js {}", jsExecQueue.front());
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
  if (browser == nullptr) {
    logger::error("browser init failed: browser is nullptr");
    return;
  }

  UpdateAll();
}
