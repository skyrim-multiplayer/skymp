#pragma once
#include "TPOverlayService.h"
#include <chrono>

class SkyrimPlatform
{
public:
  static SkyrimPlatform* GetSingleton();

  void JsTick(Napi::Env env, bool gameFunctionsAvailable);
  void AddTickTask(const std::function<void(Napi::Env env)>& f);
  void AddUpdateTask(const std::function<void(Napi::Env env)>& f);
  void PushAndWait(const std::function<void(Napi::Env env)>& task);
  // Same as PushAndWait, but gives up after 'timeout' and returns false.
  // Use this when the calling thread must not block the game forever
  // (e.g. game-side hooks running on the main thread).
  bool PushAndWaitFor(const std::function<void(Napi::Env env)>& task,
                      std::chrono::milliseconds timeout);
  void Push(const std::function<void(Napi::Env env)>& task);
  void PushToWorkerAndWait(
    RE::BSTSmartPointer<RE::BSScript::IFunction> fPtr,
    const RE::BSTSmartPointer<RE::BSScript::Stack>& stack,
    RE::BSScript::ErrorLogger* logger,
    RE::BSScript::Internal::VirtualMachine* vm,
    RE::BSScript::IFunction::CallResult* ret);
  void PrepareWorker();
  void StartWorker();
  void StopWorker();

private:
  SkyrimPlatform();
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
