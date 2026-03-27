#pragma once
#include "PapyrusTESModPlatform.h"
#include "SkyrimPlatform.h"

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
    _taskInterface->AddTask(onTick);
  }

private:
  TickHandler()
  {
    _taskInterface = SKSE::GetTaskInterface();
  }

  const std::function<void()> onTick = [] {
    SkyrimPlatform::GetSingleton()->RunTask([=](Napi::Env env) {
      SkyrimPlatform::GetSingleton()->JsTick(env, false);
    });
    TESModPlatform::Update();
    TickHandler::GetSingleton()->Update();
  };

  const SKSE::TaskInterface* _taskInterface;
};
