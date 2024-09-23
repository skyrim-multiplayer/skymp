#pragma once
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include <memory>

#include "Void.h"

namespace Viet {
template <class State = Viet::Void>
class TaskQueue
{
public:
  TaskQueue();

  void AddTask(const std::function<void(const State&)>& task);
  void Update(const State &state);
  void Clear();

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
}
