#pragma once
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class TaskQueue
{
public:
  void AddTask(std::function<void()> task)
  {
    std::lock_guard l(m);
    tasks.push_back(task);
  }

  void Update()
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
        task();
      } catch (std::exception& e) {
        // Other tasks should be executed later even if one throws
        std::lock_guard l(m);
        for (size_t j = i + 1; j < tasksCopy.size(); ++j)
          tasks.push_back(tasksCopy[j]);
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