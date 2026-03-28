#pragma once
#include "PapyrusTESModPlatform.h"
#include "SkyrimPlatform.h"
#include "ThreadPoolWrapper.h"

class TickHandler
{
public:
  [[nodiscard]] static TickHandler* GetSingleton()
  {
    static TickHandler singleton;
    return &singleton;
  }

  void Update()
  {
    // Must be async to avoid infinite loop in SKSE's task drain.
    // Synchronous AddTask from within a task callback causes SKSE to
    // immediately re-process the newly added task in the same frame.
    _threadPool.Push([this] { _taskInterface->AddTask(onTick); });
  }

private:
  TickHandler() { _taskInterface = SKSE::GetTaskInterface(); }

  const std::function<void()> onTick = [] {
    SkyrimPlatform::GetSingleton()->RunTask([=](Napi::Env env) {
      SkyrimPlatform::GetSingleton()->JsTick(env, false);
    });
    TESModPlatform::Update();
    TickHandler::GetSingleton()->Update();
  };

  const SKSE::TaskInterface* _taskInterface;
  ThreadPoolWrapper _threadPool{ 1 };
};
