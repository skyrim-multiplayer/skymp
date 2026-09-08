#include "ThreadPool.h"

#include <algorithm>
#include <spdlog/spdlog.h>

#if defined(_MSC_VER)
#  include <intrin.h>
#elif defined(__i386__) || defined(__x86_64__)
#  include <immintrin.h>
#endif

namespace MpParallel {

void ThreadPool::CpuRelax() noexcept
{
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
  _mm_pause();
#elif defined(_MSC_VER) && defined(_M_ARM64)
  __yield();
#elif defined(__i386__) || defined(__x86_64__)
  _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
  __asm__ __volatile__("yield");
#else
  std::this_thread::yield();
#endif
}

ThreadPool::ThreadPool(size_t numWorkers, uint32_t spinMicros_)
  : spinMicros(spinMicros_)
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
    stopping.store(true, std::memory_order_release);
    // Bump the prime token too, so a worker parked on it leaves its wait.
    primeToken.fetch_add(1, std::memory_order_relaxed);
  }
  cvWork.notify_all();

  for (auto& worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::Prime(size_t expectedTasks)
{
  if (workers.empty() || spinMicros == 0 || expectedTasks <= 1) {
    return;
  }

  // The calling thread is one of the slots, so a batch of N tasks needs at
  // most N-1 workers.
  const size_t needed = std::min(expectedTasks - 1, workers.size());
  size_t toWake = 0;

  {
    std::lock_guard<std::mutex> lock(mutex);
    primeToken.fetch_add(1, std::memory_order_relaxed);
    // Nobody can enter or leave cvWork.wait without this lock, so this count
    // is exact.
    toWake = std::min(needed - std::min(needed, WorkersAwake()),
                      blockedWorkers);
  }

  // notify_one rather than notify_all: waking thirty threads to run four
  // tasks is most of what made the offload lose at low population, and the
  // spin they do afterwards is CPU taken from the main thread's ingest.
  for (size_t i = 0; i < toWake; ++i) {
    cvWork.notify_one();
  }
}

void ThreadPool::Run(const std::vector<Task>& tasks)
{
  if (tasks.empty()) {
    return;
  }

  // One task is not worth a barrier: handing a worker the only piece of work
  // costs strictly more than running it here. This is the common shape at low
  // population, where the dispatcher deliberately stops splitting once a shard
  // would be too small to pay for its own dispatch.
  if (workers.empty() || tasks.size() == 1) {
    for (size_t i = 0; i < tasks.size(); ++i) {
      RunTaskGuarded(tasks[i], 0, i);
    }
    return;
  }

  const auto count = static_cast<uint32_t>(tasks.size());

  // Safe without synchronisation: a straggler from the previous batch can
  // only read `currentTasks` after claiming an index from a cursor whose
  // generation still matches its own, and the previous batch's generation is
  // about to stop matching anything.
  currentTasks.store(&tasks, std::memory_order_relaxed);
  completedTasks.store(0, std::memory_order_relaxed);
  taskCount.store(count, std::memory_order_relaxed);

  const uint32_t generation = GenerationOf(cursor.load(
                                std::memory_order_relaxed)) +
    1;

  bool notifyWorkers = false;
  size_t toWake = 0;
  {
    // Publishing under the mutex even though the cursor is an atomic: a
    // worker evaluates its wait predicate while holding this lock, and
    // publishing outside it would leave the classic lost-wakeup window
    // between that evaluation and the wait itself.
    std::lock_guard<std::mutex> lock(mutex);
    cursor.store(MakeCursor(generation, 0), std::memory_order_release);
    // Only the shortfall. A worker that Prime already woke is spinning on
    // this very cursor and needs nothing further; waking a blocked one in its
    // place just to find the batch already claimed is a thread wakeup spent
    // on nothing, and at one per tick per surplus worker that was measurable.
    const size_t needed = count - 1;
    toWake = std::min<size_t>(needed - std::min<size_t>(needed, WorkersAwake()),
                              blockedWorkers);
    notifyWorkers = toWake > 0;
  }
  if (notifyWorkers) {
    for (size_t i = 0; i < toWake; ++i) {
      cvWork.notify_one();
    }
  }

  // The caller is slot 0 and claims from the same cursor as everybody else.
  DrainTasks(0, generation);

  const auto allDone = [this, count] {
    return completedTasks.load(std::memory_order_acquire) == count;
  };

  // Whoever ran the last task may still be inside its claim loop, but it can
  // no longer touch anything of ours: the batch is exhausted, so its next
  // claim fails the index test, and the next batch's generation will fail the
  // generation test.
  if (!SpinUntil(allDone)) {
    std::unique_lock<std::mutex> lock(mutex);
    runnerBlocked = true;
    cvDone.wait(lock, allDone);
    runnerBlocked = false;
  }

  currentTasks.store(nullptr, std::memory_order_relaxed);
}

void ThreadPool::WorkerMain(size_t slotIndex)
{
  uint64_t seenPrime = 0;
  uint32_t seenGeneration = GenerationOf(cursor.load(std::memory_order_relaxed));

  const auto batchReady = [this, &seenGeneration] {
    return GenerationOf(cursor.load(std::memory_order_acquire)) !=
      seenGeneration;
  };
  const auto isStopping = [this] {
    return stopping.load(std::memory_order_acquire);
  };

  for (;;) {
    {
      std::unique_lock<std::mutex> lock(mutex);
      ++blockedWorkers;
      cvWork.wait(lock, [&] {
        return batchReady() || isStopping() ||
          primeToken.load(std::memory_order_relaxed) != seenPrime;
      });
      --blockedWorkers;
      seenPrime = primeToken.load(std::memory_order_relaxed);
    }

    // Woken by a prime rather than by a batch: the main thread is still
    // assembling one out of this tick's packets. Staying hot across that gap
    // is the whole point -- it moves the wakeup off the critical path, and it
    // costs at most the ingest duration rather than the tick period, because
    // once the budget expires we park again.
    if (!batchReady() && !isStopping()) {
      (void)SpinUntil([&] { return batchReady() || isStopping(); });
    }

    if (isStopping()) {
      return;
    }
    if (!batchReady()) {
      // The prime expired without a batch behind it. Nothing to do.
      continue;
    }

    seenGeneration = GenerationOf(cursor.load(std::memory_order_acquire));
    DrainTasks(slotIndex, seenGeneration);
  }
}

void ThreadPool::DrainTasks(size_t slotIndex, uint32_t batchGeneration)
{
  const uint32_t count = taskCount.load(std::memory_order_relaxed);

  for (;;) {
    uint64_t current = cursor.load(std::memory_order_acquire);

    // Either the batch moved on without us or it is exhausted. Both mean
    // there is nothing here, and neither may touch `currentTasks`.
    if (GenerationOf(current) != batchGeneration) {
      return;
    }
    const uint32_t index = IndexOf(current);
    if (index >= count) {
      return;
    }

    if (!cursor.compare_exchange_weak(current, current + 1,
                                      std::memory_order_acq_rel,
                                      std::memory_order_relaxed)) {
      continue;
    }

    // The index is ours and Run is still waiting on it -- it cannot have
    // returned, because completedTasks is short by at least this task -- so
    // the vector is alive for as long as it takes to run.
    //
    // The null check is unreachable by that same argument, and is written as
    // "skip the work, still account for the claim" rather than as a bail-out:
    // once an index has been taken off the cursor, nothing else will ever run
    // it, so returning here instead would leave Run waiting on a task that
    // can no longer complete.
    if (const std::vector<Task>* tasks =
          currentTasks.load(std::memory_order_relaxed)) {
      RunTaskGuarded((*tasks)[index], slotIndex, index);
    }

    const uint32_t done =
      completedTasks.fetch_add(1, std::memory_order_acq_rel) + 1;
    if (done == count) {
      // Reading runnerBlocked under the mutex closes the window where Run has
      // evaluated its predicate but has not yet re-entered the wait.
      bool notify = false;
      {
        std::lock_guard<std::mutex> lock(mutex);
        notify = runnerBlocked;
      }
      if (notify) {
        cvDone.notify_one();
      }
      return;
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
