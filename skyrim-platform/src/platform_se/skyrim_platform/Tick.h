#pragma once
#include "PapyrusTESModPlatform.h"
#include "SkyrimPlatform.h"
#include "ThreadPoolWrapper.h"

class Tick
{
public:
  [[nodiscard]] static Tick* GetSingleton()
  {
    static Tick singleton;
    return &singleton;
  }

  void Update()
  {
    _threadPool->Push([=](int) { _taskInterface->AddTask(_onTick); });
  }

private:
  Tick()
  {
    _taskInterface = SKSE::GetTaskInterface();
    _threadPool = std::make_shared<ThreadPoolWrapper>();
    _onTick = [] {
      SkyrimPlatform::GetSingleton().PushAndWait(
        [=](int) { SkyrimPlatform::GetSingleton().JsTick(false); });
      TESModPlatform::Update();
      Tick::GetSingleton()->Update();
    };
  }

  const SKSE::TaskInterface* _taskInterface;
  std::shared_ptr<ThreadPoolWrapper> _threadPool;
  std::function<void()> _onTick;
};
