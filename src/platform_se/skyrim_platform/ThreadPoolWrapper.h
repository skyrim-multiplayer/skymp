#pragma once
#include <ctpl/ctpl_stl.h>
#include <memory>

class ThreadPoolWrapper
{
public:
  ThreadPoolWrapper(int numThreads_ = 1)
    : numThreads(numThreads_)
  {
  }

  auto Push(std::function<void(int)> f)
  {
    if (!pool)
      pool.reset(new ctpl::thread_pool(numThreads));
    return pool->push(f);
  }

private:
  std::unique_ptr<ctpl::thread_pool> pool;
  const int numThreads;
};