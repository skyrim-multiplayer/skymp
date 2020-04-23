#pragma once
#include "ctpl/ctpl_stl.h"
#include <skse64/PluginAPI.h>
#include <skse64/gamethreads.h>

class MyUpdateTask : public TaskDelegate
{
public:
  using OnUpdate = std::function<void()>;

  MyUpdateTask(SKSETaskInterface* taskInterface, const OnUpdate& onUpdate)
  {
    state.taskInterface = taskInterface;
    state.threadPool.reset(new ctpl::thread_pool(1));
    state.onUpdate = onUpdate;
  }

private:
  struct State
  {
    SKSETaskInterface* taskInterface = nullptr;
    std::shared_ptr<ctpl::thread_pool> threadPool;
    OnUpdate onUpdate;
  } state;

  MyUpdateTask(const State& state_) { state = state_; }

  void Run() override
  {
    if (state.onUpdate)
      state.onUpdate();

    auto state = this->state;
    state.threadPool->push(
      [state](int) { state.taskInterface->AddTask(new MyUpdateTask(state)); });
  }

  void Dispose() override { delete this; }
};