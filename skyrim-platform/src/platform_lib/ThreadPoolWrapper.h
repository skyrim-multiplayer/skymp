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

  auto Push(std::function<void(int32_t)> f)
  {
    if (!pool) {
      pool = std::make_unique<ctpl::thread_pool>(numThreads);
    }
    return pool->push(f);
  }

private:
  std::unique_ptr<ctpl::thread_pool> pool;
  const int32_t numThreads;
};
