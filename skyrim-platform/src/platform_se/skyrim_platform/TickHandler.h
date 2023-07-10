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
    _threadPool->Push([=] { _taskInterface->AddTask(onTick); });
  }

private:
  TickHandler()
  {
    _taskInterface = SKSE::GetTaskInterface();
    _threadPool = std::make_shared<ThreadPoolWrapper>();
  }

  const std::function<void()> onTick = [] {
    SkyrimPlatform::GetSingleton()->PushAndWait(
      [=] { SkyrimPlatform::GetSingleton()->JsTick(false); });
    TESModPlatform::Update();
    TickHandler::GetSingleton()->Update();
  };

  const SKSE::TaskInterface* _taskInterface;
  std::shared_ptr<ThreadPoolWrapper> _threadPool;
};
