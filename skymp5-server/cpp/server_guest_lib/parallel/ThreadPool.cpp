#include "ThreadPool.h"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace MpParallel {

ThreadPool::ThreadPool(size_t numWorkers)
{
  workers.reserve(numWorkers);
  for (size_t i = 0; i < numWorkers; ++i) {
    // Slot 0 belongs to the calling thread, so spawned workers start at 1.
    workers.emplace_back([this, slotIndex = i + 1] { WorkerMain(slotIndex); });
  }
}

ThreadPool::~ThreadPool()
{
  {
    std::lock_guard<std::mutex> lock(mutex);
    stopping = true;
    // Bump the generation as well so a worker that has not yet observed the
    // most recent batch still leaves its wait.
    ++generation;
  }
  cvWork.notify_all();

  for (auto& worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::Run(const std::vector<Task>& tasks)
{
  if (tasks.empty()) {
    return;
  }

  if (workers.empty()) {
    for (size_t i = 0; i < tasks.size(); ++i) {
      RunTaskGuarded(tasks[i], 0, i);
    }
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mutex);
    currentTasks = &tasks;
    nextTaskIndex.store(0, std::memory_order_relaxed);
    tasksRemaining.store(tasks.size(), std::memory_order_relaxed);
    ++generation;
  }
  cvWork.notify_all();

  // The caller is slot 0 and pulls from the same cursor as everybody else.
  DrainTasks(tasks, 0);

  {
    std::unique_lock<std::mutex> lock(mutex);
    // Both conditions are required: the first says the work is done, the
    // second says nobody is still touching the cursor or the task vector.
    // See the comment on activeDrainers.
    cvDone.wait(lock, [this] {
      return tasksRemaining.load(std::memory_order_acquire) == 0 &&
        activeDrainers == 0;
    });
    // Cleared under the lock so a worker that wakes late reads nullptr rather
    // than a pointer to the caller's vector, which may die once Run returns.
    currentTasks = nullptr;
  }
}

void ThreadPool::WorkerMain(size_t slotIndex)
{
  uint64_t seenGeneration = 0;

  for (;;) {
    const std::vector<Task>* tasks = nullptr;

    {
      std::unique_lock<std::mutex> lock(mutex);
      cvWork.wait(lock,
                  [&] { return stopping || generation != seenGeneration; });
      if (stopping) {
        return;
      }
      // Reading the generation and the batch pointer under the same lock is
      // what makes a late waker safe: it either misses a batch entirely or
      // sees a matching (generation, tasks) pair, never a torn one.
      seenGeneration = generation;
      tasks = currentTasks;
      if (tasks) {
        // Registered before releasing the lock so Run's predicate can never
        // observe "no drainers" for a worker that is about to start.
        ++activeDrainers;
      }
    }

    if (tasks) {
      DrainTasks(*tasks, slotIndex);

      {
        std::lock_guard<std::mutex> lock(mutex);
        --activeDrainers;
      }
      // The last drainer to leave may be what Run is waiting on, even when
      // tasksRemaining reached zero some time ago.
      cvDone.notify_all();
    }
  }
}

void ThreadPool::DrainTasks(const std::vector<Task>& tasks, size_t slotIndex)
{
  const size_t taskCount = tasks.size();

  for (;;) {
    const size_t taskIndex =
      nextTaskIndex.fetch_add(1, std::memory_order_relaxed);
    if (taskIndex >= taskCount) {
      return;
    }

    RunTaskGuarded(tasks[taskIndex], slotIndex, taskIndex);

    if (tasksRemaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      // Taking the mutex before notifying closes the window where the waiter
      // has evaluated its predicate but has not yet re-entered the wait.
      std::lock_guard<std::mutex> lock(mutex);
      cvDone.notify_all();
    }
  }
}

void ThreadPool::RunTaskGuarded(const Task& task, size_t slotIndex,
                                size_t taskIndex) noexcept
{
  if (!task) {
    return;
  }

  try {
    task(slotIndex);
  } catch (const std::exception& e) {
    failedTaskCount.fetch_add(1, std::memory_order_relaxed);
    // spdlog is thread-safe; this is the only logging call on the parallel
    // path and it only fires on the error branch.
    spdlog::error("MpParallel::ThreadPool - task {} on slot {} threw: {}",
                  taskIndex, slotIndex, e.what());
  } catch (...) {
    failedTaskCount.fetch_add(1, std::memory_order_relaxed);
    spdlog::error(
      "MpParallel::ThreadPool - task {} on slot {} threw a non-std exception",
      taskIndex, slotIndex);
  }
}

}
