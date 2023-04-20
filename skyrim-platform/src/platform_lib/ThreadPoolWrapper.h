#pragma once
#include <BS_thread_pool_light.hpp>
#include <cstdint>
#include <memory>
#include <mutex>

class ThreadPoolWrapper
{
public:
  ThreadPoolWrapper(int32_t numThreads_ = 1)
    : numThreads(numThreads_)
  {
  }

  std::future<void> Push(const std::function<void()>& task)
  {
    {
      std::lock_guard l(initMutex);
      if (!pool) {
        pool = std::make_unique<BS::thread_pool_light>(numThreads);
      }
    }
    return pool->submit(task);
  }

  void PushAndWait(const std::function<void()>& task) { Push(task).wait(); }

private:
  std::unique_ptr<BS::thread_pool_light> pool;
  const int32_t numThreads;
  std::mutex initMutex;
};
