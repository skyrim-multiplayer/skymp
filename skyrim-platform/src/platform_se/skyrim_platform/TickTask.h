#pragma once
#include "ThreadPoolWrapper.h"
#include <skse64/PluginAPI.h>
#include <skse64/gamethreads.h>

class TickTask : public TaskDelegate
{
public:
  TickTask(SKSETaskInterface* taskInterface,
           const std::function<void()>& onTick)
  {
    state.taskInterface = taskInterface;
    state.threadPool = std::make_shared<ThreadPoolWrapper>();
    state.onTick = onTick;
  }

private:
  struct State
  {
    SKSETaskInterface* taskInterface = nullptr;
    std::shared_ptr<ThreadPoolWrapper> threadPool;
    std::function<void()> onTick;
  } state;

  TickTask(const State& state_) { state = state_; }

  void Run() override
  {
    if (state.onTick) {
      state.onTick();
    }

    auto state = this->state;
    state.threadPool->Push(
      [state](int) { state.taskInterface->AddTask(new TickTask(state)); });
  }

  void Dispose() override { delete this; }
};
