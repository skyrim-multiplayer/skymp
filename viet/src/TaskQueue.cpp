#include "TaskQueue.h"

struct Viet::TaskQueue::Impl
{
  std::mutex m;
  std::vector<std::function<void()>> tasks;
};

Viet::TaskQueue::TaskQueue()
{
  pImpl = std::make_shared<Impl>();
}

void Viet::TaskQueue::AddTask(const std::function<void(const State &)>& task)
{
  std::lock_guard l(pImpl->m);
  pImpl->tasks.push_back(task);
}

void Viet::TaskQueue::Update(const State &state)
{
  decltype(pImpl->tasks) tasksCopy;
  {
    std::lock_guard l(pImpl->m);
    tasksCopy = std::move(pImpl->tasks);
    pImpl->tasks.clear();
  }

  for (size_t i = 0; i < tasksCopy.size(); ++i) {
    auto& task = tasksCopy[i];
    try {
      task(state);
    } catch (const std::exception&) {
      // Other tasks should be executed later even if one throws
      std::lock_guard l(pImpl->m);
      for (size_t j = i + 1; j < tasksCopy.size(); ++j) {
        pImpl->tasks.push_back(tasksCopy[j]);
      }
      throw;
    }
  }
}

void Viet::TaskQueue::Clear()
{
  std::lock_guard l(pImpl->m);
  pImpl->tasks.clear();
}
