#pragma once
#include "ThreadPoolWrapper.h"
#include <skse64/PluginAPI.h>
#include <skse64/gamethreads.h>

class MyUpdateTask : public TaskDelegate
{
public:
  using OnUpdate = std::function<void()>;

  MyUpdateTask(SKSETaskInterface* taskInterface, const OnUpdate& onUpdate)
  {
    state.taskInterface = taskInterface;
    state.threadPool.reset(new ThreadPoolWrapper);
    state.onUpdate = onUpdate;
  }

private:
  struct State
  {
    SKSETaskInterface* taskInterface = nullptr;
    std::shared_ptr<ThreadPoolWrapper> threadPool;
    OnUpdate onUpdate;
  } state;

  MyUpdateTask(const State& state_) { state = state_; }

  void Run() override
  {
    if (state.onUpdate)
      state.onUpdate();

    auto state = this->state;
    state.threadPool->Push(
      [state](int) { state.taskInterface->AddTask(new MyUpdateTask(state)); });
  }

  void Dispose() override { delete this; }
};