#pragma once
#include <cstdint>
#include <ctpl/ctpl_stl.h>
#include <memory>

class ThreadPoolWrapper
{
public:
  ThreadPoolWrapper(int32_t numThreads_ = 1)
    : numThreads(numThreads_)
  {
  }

  std::future<void> Push(const std::function<void(int32_t)>& task)
  {
    if (!pool) {
      pool = std::make_unique<ctpl::thread_pool>(numThreads);
    }
    return pool->push(task);
  }

  void PushAndWait(const std::function<void(int32_t)>& task)
  {
    Push(task).wait();
  }

private:
  std::unique_ptr<ctpl::thread_pool> pool;
  const int32_t numThreads;
};
