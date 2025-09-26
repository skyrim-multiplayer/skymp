#pragma once

#include <deque>

#include "NirnLabUIPlatformAPI/API.h"

class BrowserApiNirnLab
{
public:
  // TODO: make non-static
  static void Register(Napi::Env env, Napi::Object& exports);
  void HandleSkseMessage(SKSE::MessagingInterface::Message* a_msg);
  // ???
  static void HandleNirnLabMessage(SKSE::MessagingInterface::Message* a_msg);
  static BrowserApiNirnLab& GetInstance();

private:
  static std::unique_ptr<BrowserApiNirnLab> g_instance;
  struct NirnLabApiHolder
  {
    bool versionChecked = false;
    NL::UI::IUIPlatformAPI* api = nullptr;

    bool Ready() const {
      return api != nullptr && versionChecked;
    }

    NL::UI::IUIPlatformAPI* operator->() const {
      // if (api == nullptr) {
      //   throw std::runtime_error("NirnLab UI API is not ready");
      // }
      // if (!versionChecked) {
      //   throw std::runtime_error("NirnLab UI API version mismatch");
      // }
      //return versionChecked ? api : nullptr;
      return api;
    }
  };

  NirnLabApiHolder api;
  NL::CEF::IBrowser* browser;
  // NL::UI::IUIPlatformAPI* api;
  
  bool wantedIsVisible;
  bool wantedIsFocused;
  std::string wantedUrl;
  std::deque<std::string> jsExecQueue;

private:
  Napi::Value SetVisible(const Napi::CallbackInfo& info);
  Napi::Value IsVisible(const Napi::CallbackInfo& info);
  Napi::Value SetFocused(const Napi::CallbackInfo& info);
  Napi::Value IsFocused(const Napi::CallbackInfo& info);
  Napi::Value LoadUrl(const Napi::CallbackInfo& info);
  Napi::Value GetToken(const Napi::CallbackInfo& info);
  Napi::Value ExecuteJavaScript(const Napi::CallbackInfo& info);

private:
  void UpdateVisible();
  void UpdateFocused();
  void UpdateUrl();
  void UpdateJs();
  void UpdateAll();
  void ApiInit();
};
