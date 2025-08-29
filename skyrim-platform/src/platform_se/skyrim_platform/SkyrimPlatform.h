#pragma once
#include "TPOverlayService.h"

#include <future>

class SkyrimPlatform
{
public:
  static SkyrimPlatform* GetSingleton();

  void JsTick(Napi::Env env, bool gameFunctionsAvailable);
  void SetOverlayService(std::shared_ptr<OverlayService> overlayService);
  void AddTickTask(const std::function<void(Napi::Env env)>& f);
  void AddUpdateTask(const std::function<void(Napi::Env env)>& f);

  // Must be called in main game thread
  void RunTask(const std::function<void(Napi::Env env)>& task);

  // Must be called from non-main game thread (i.e. Anim event hook)
  std::future<void> QueueTask(const std::function<void(Napi::Env env)>& task);

  void PushToWorkerAndWait(
    RE::BSTSmartPointer<RE::BSScript::IFunction> fPtr,
    const RE::BSTSmartPointer<RE::BSScript::Stack>& stack,
    RE::BSScript::ErrorLogger* logger,
    RE::BSScript::Internal::VirtualMachine* vm,
    RE::BSScript::IFunction::CallResult* ret);

private:
  SkyrimPlatform();
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
