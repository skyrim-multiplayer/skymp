#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace MpParallel {

// A fork/join pool sized once at startup.
//
// Two properties matter for this use case and drive the design:
//
//  * The calling (main) thread participates in the work instead of blocking
//    idle. With small batches the barrier cost dominates, and leaving a core
//    parked makes the offload lose to the inline path.
//
//  * Tasks are handed out by an atomic cursor rather than pre-partitioned.
//    Area clusters differ in cost by an order of magnitude (a city square vs
//    a lone traveller), so static partitioning would leave workers idle at
//    the barrier.
//
// Each task receives a worker index in [0, GetSlotCount()). Slot 0 is always
// the calling thread. Callers use the index to reach per-slot scratch
// buffers, which is what keeps the parallel phase allocation-free and
// lock-free.
class ThreadPool
{
public:
  using Task = std::function<void(size_t slotIndex)>;

  // numWorkers is the number of *additional* threads spawned. A value of 0
  // produces a valid pool that runs everything on the calling thread, which
  // is the mode unit tests and single-core hosts use.
  explicit ThreadPool(size_t numWorkers);

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  ~ThreadPool();

  // Number of distinct slot indices a task may observe: spawned workers plus
  // the calling thread.
  [[nodiscard]] size_t GetSlotCount() const noexcept
  {
    return workers.size() + 1;
  }

  [[nodiscard]] size_t GetWorkerCount() const noexcept
  {
    return workers.size();
  }

  // Runs every task and returns once all of them have completed. `tasks` must
  // outlive the call. Exceptions thrown by a task are caught, counted, and
  // logged; they never escape into the tick loop and never wedge the barrier.
  //
  // Not reentrant and not concurrent: exactly one thread may be inside Run at
  // a time, and a task must not call Run. The dispatcher satisfies this by
  // only ever calling it from the tick.
  void Run(const std::vector<Task>& tasks);

  // Number of tasks that threw since construction. Used by tests and by the
  // metrics snapshot.
  [[nodiscard]] uint64_t GetFailedTaskCount() const noexcept
  {
    return failedTaskCount.load(std::memory_order_relaxed);
  }

private:
  void WorkerMain(size_t slotIndex);
  void DrainTasks(const std::vector<Task>& tasks, size_t slotIndex);
  void RunTaskGuarded(const Task& task, size_t slotIndex,
                      size_t taskIndex) noexcept;

  std::vector<std::thread> workers;

  std::mutex mutex;
  std::condition_variable cvWork;
  std::condition_variable cvDone;

  // Guarded by `mutex` for publication; the atomics below are also read
  // outside the lock during draining.
  const std::vector<Task>* currentTasks = nullptr;
  uint64_t generation = 0;
  bool stopping = false;

  // Workers currently inside DrainTasks. Run must not return while this is
  // non-zero.
  //
  // Waiting on tasksRemaining alone is not enough. The worker that runs the
  // final task decrements tasksRemaining to zero and only then loops back to
  // the cursor to discover the batch is exhausted. If Run returned in that
  // window and the next tick started a batch, the reset of nextTaskIndex
  // would hand that still-draining worker index 0 of the *previous* task
  // vector: the old task would run a second time and the new batch's
  // accounting would be decremented by a task that was never part of it.
  size_t activeDrainers = 0;

  std::atomic<size_t> nextTaskIndex{ 0 };
  std::atomic<size_t> tasksRemaining{ 0 };
  std::atomic<uint64_t> failedTaskCount{ 0 };
};

}
