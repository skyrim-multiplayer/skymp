#pragma once
#include <BS_thread_pool.hpp>
#include <chrono>
#include <cstdint>
#include <future>
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
        pool = std::make_unique<BS::light_thread_pool>(numThreads);
      }
    }
    return pool->submit_task(task);
  }

  void PushAndWait(const std::function<void()>& task) { Push(task).wait(); }

  // Same as PushAndWait, but gives up after 'timeout' and returns false.
  // Use this from the main game thread so a slow or stuck task on the
  // pool can't freeze the whole game -- the caller can decide to just
  // move on when this returns false.
  //
  // Note: on timeout the task is NOT cancelled, it stays queued and may
  // still run later. Any state it captures must therefore stay valid for
  // the whole lifetime of the pool (typical solution: capture by value
  // into a shared_ptr).
  bool PushAndWaitFor(const std::function<void()>& task,
                      std::chrono::milliseconds timeout)
  {
    auto future = Push(task);
    return future.wait_for(timeout) == std::future_status::ready;
  }

private:
  std::unique_ptr<BS::light_thread_pool> pool;
  const int32_t numThreads;
  std::mutex initMutex;
};
