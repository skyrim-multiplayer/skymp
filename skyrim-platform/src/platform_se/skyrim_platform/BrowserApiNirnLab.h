#pragma once

#include <deque>

#include "NirnLabUIPlatformAPI/API.h"

class BrowserApiNirnLab
{
public:
  void HandleSkseMessage(SKSE::MessagingInterface::Message* a_msg);
  static void HandleNirnLabMessage(SKSE::MessagingInterface::Message* a_msg);
  static BrowserApiNirnLab& GetInstance();

private:
  static std::unique_ptr<BrowserApiNirnLab> g_instance;
  struct NirnLabApiHolder
  {
    bool versionChecked = false;
    NL::UI::IUIPlatformAPI* api = nullptr;

    bool Ready() const { return api != nullptr && versionChecked; }

    NL::UI::IUIPlatformAPI* operator->() const { return api; }
  };

  NirnLabApiHolder api;
  NL::CEF::IBrowser* browser;

  bool wantedIsVisible;
  bool wantedIsFocused;
  std::string wantedUrl;
  std::deque<std::string> jsExecQueue;

private:
  Napi::Value GetBackend(const Napi::CallbackInfo& info);
  Napi::Value SetVisible(const Napi::CallbackInfo& info);
  Napi::Value IsVisible(const Napi::CallbackInfo& info);
  Napi::Value SetFocused(const Napi::CallbackInfo& info);
  Napi::Value IsFocused(const Napi::CallbackInfo& info);
  Napi::Value LoadUrl(const Napi::CallbackInfo& info);
  Napi::Value GetToken(const Napi::CallbackInfo& info);
  Napi::Value ExecuteJavaScript(const Napi::CallbackInfo& info);

  void UpdateVisible();
  void UpdateFocused();
  void UpdateUrl();
  void UpdateJs();
  void UpdateAll();
  void ApiInit();
};
