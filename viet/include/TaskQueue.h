#pragma once
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include <memory>

namespace Viet {
class TaskQueue
{
public:
  TaskQueue();

  void AddTask(const std::function<void()>& task);
  void Update();
  void Clear();

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
}
