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

  void AddTask(const std::function<void(const State&)>& task)
  {
    std::lock_guard l(m);
    tasks.push_back(task);
  }

  void Update(const State &state)
  {
    decltype(tasks) tasksCopy;
    {
      std::lock_guard l(m);
      tasksCopy = std::move(tasks);
      tasks.clear();
    }

    for (size_t i = 0; i < tasksCopy.size(); ++i) {
      auto& task = tasksCopy[i];
      try {
        task(state);
      } catch (const std::exception&) {
        // Other tasks should be executed later even if one throws
        std::lock_guard l(m);
        for (size_t j = i + 1; j < tasksCopy.size(); ++j) {
          tasks.push_back(tasksCopy[j]);
        }
        throw;
      }
    }
  }

  void Clear()
  {
    std::lock_guard l(m);
    tasks.clear();
  }

private:
  std::mutex m;
  std::vector<std::function<void()>> tasks;
};
}
