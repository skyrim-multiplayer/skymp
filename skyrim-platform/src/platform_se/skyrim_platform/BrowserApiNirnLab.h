#pragma once

#include <deque>

#include "NirnLabUIPlatformAPI/API.h"

class BrowserApiNirnLab
{
public:
  void HandleSkseMessage(SKSE::MessagingInterface::Message* a_msg);
  static void HandleNirnLabMessage(SKSE::MessagingInterface::Message* a_msg);
  static BrowserApiNirnLab& GetInstance();

  Napi::Value SetVisible(const Napi::CallbackInfo& info);
  Napi::Value IsVisible(const Napi::CallbackInfo& info);
  Napi::Value SetFocused(const Napi::CallbackInfo& info);
  Napi::Value IsFocused(const Napi::CallbackInfo& info);
  Napi::Value LoadUrl(const Napi::CallbackInfo& info);
  Napi::Value GetToken(const Napi::CallbackInfo& info);
  Napi::Value ExecuteJavaScript(const Napi::CallbackInfo& info);

private:
  BrowserApiNirnLab() = default;

  void UpdateVisible();
  void UpdateFocused();
  void UpdateUrl();
  void UpdateJs();
  void UpdateAll();
  void ApiInit();

  struct NirnLabApiHolder
  {
    bool versionChecked = false;
    NL::UI::IUIPlatformAPI* api = nullptr;

    bool Ready() const { return api != nullptr && versionChecked; }

    NL::UI::IUIPlatformAPI* operator->() const { return api; }
  };

  NirnLabApiHolder api;
  NL::CEF::IBrowser* browser = nullptr;
  bool wantedIsVisible = false;
  bool wantedIsFocused = false;
  std::string wantedUrl;
  std::deque<std::string> jsExecQueue;

  static std::unique_ptr<BrowserApiNirnLab> g_instance;
};
