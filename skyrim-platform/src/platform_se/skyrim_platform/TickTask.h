#pragma once
#include "ThreadPoolWrapper.h"
#include <skse64/PluginAPI.h>
#include <skse64/gamethreads.h>

class TickTask : public TaskDelegate
{
public:
  static void Launch(SKSETaskInterface* taskInterface,
                     const std::function<void()>& onTick)
  {
    taskInterface->AddTask(new TickTask(taskInterface, onTick));
  }

private:
  struct State
  {
    SKSETaskInterface* taskInterface = nullptr;
    std::shared_ptr<ThreadPoolWrapper> threadPool;
    std::function<void()> onTick;
  };

  TickTask(SKSETaskInterface* taskInterface,
           const std::function<void()>& onTick)
  {
    state.taskInterface = taskInterface;
    state.threadPool = std::make_shared<ThreadPoolWrapper>();
    state.onTick = onTick;
  }

  explicit TickTask(const State& state_) { state = state_; }

  void Run() override
  {
    state.onTick();

    state.threadPool->Push([state = this->state](int) {
      state.taskInterface->AddTask(new TickTask(state));
    });
  }

  void Dispose() override { delete this; }

  State state;
};
