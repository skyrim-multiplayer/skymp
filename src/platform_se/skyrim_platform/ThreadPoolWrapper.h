#pragma once
#include <ctpl/ctpl_stl.h>
#include <memory>

class ThreadPoolWrapper
{
public:
  auto Push(std::function<void(int)> f)
  {
    if (!pool)
      pool.reset(new ctpl::thread_pool(1));
    return pool->push(f);
  }

private:
  std::unique_ptr<ctpl::thread_pool> pool;
};