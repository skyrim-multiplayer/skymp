#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace MpParallel {

// How long a primed worker stays hot waiting for the batch to arrive.
//
// This number is the difference between the offload being worth enabling at
// 150 players and not. A condition-variable round trip costs tens of
// microseconds once several threads have to be woken and rescheduled, and at
// that population the whole parallel phase is only about 140us of work:
// measured, the barrier alone was ~50us of a 64us parallel phase.
//
// Workers do not spin between ticks -- that would burn a core each for the
// millisecond the server spends sleeping. They spin only across the gap
// between Prime() and Run(), which is the packet ingest the batch is being
// assembled from, so the cost is bounded by ingest time rather than by the
// tick period. And only as many of them spin as Prime was told to expect, so
// a quiet tick does not put thirty cores into a pause loop next to a main
// thread that is trying to parse packets.
//
// Set to 0 to restore pure blocking behaviour.
constexpr uint32_t kDefaultSpinMicros = 250;

// A fork/join pool sized once at startup.
//
// Four properties matter for this use case and drive the design:
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
//  * Only as many workers as there is work for ever get involved. Run waits
//    on the tasks, not on the threads, so a four-task batch costs four
//    wakeups on a thirty-worker pool rather than thirty.
//
//  * Workers can be told a batch is coming (Prime) before it exists. Waking
//    them at the start of packet ingest moves the wakeup off the critical
//    path: by the time Run publishes, they are already spinning on the
//    cursor and pick it up in nanoseconds.
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
  explicit ThreadPool(size_t numWorkers,
                      uint32_t spinMicros = kDefaultSpinMicros);

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

  [[nodiscard]] uint32_t GetSpinMicros() const noexcept { return spinMicros; }

  // Tells up to `expectedTasks` parked workers that a batch is coming, so
  // they wake now and spin rather than being woken when Run needs them
  // immediately.
  //
  // Call it as early as the caller knows work is coming -- for the movement
  // offload that is the first submission of the tick, tens to hundreds of
  // microseconds before Run. `expectedTasks` need only be approximate; the
  // previous tick's shard count is a good estimate. Priming more workers than
  // the batch turns out to need costs a spin each, priming fewer costs a
  // wakeup on the critical path for the rest.
  //
  // Calling it and then not calling Run is harmless: the workers spin out
  // their budget and park again.
  //
  // Optional. Without it Run still wakes whoever it needs, which is the
  // behaviour this pool had before Prime existed.
  void Prime(size_t expectedTasks);

  // Runs every task and returns once all of them have completed. `tasks` must
  // outlive the call. Exceptions thrown by a task are caught, counted, and
  // logged; they never escape into the tick loop and never wedge the barrier.
  //
  // A batch of one task never touches the barrier at all: it runs on the
  // calling thread, because handing a worker the only piece of work costs
  // more than doing it here.
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

  // Claims and runs tasks from `batchGeneration` until the batch is
  // exhausted or superseded. Touches `currentTasks` only after it has
  // successfully claimed an index, which is what makes a straggler safe: see
  // the comment on `cursor`.
  void DrainTasks(size_t slotIndex, uint32_t batchGeneration);

  void RunTaskGuarded(const Task& task, size_t slotIndex,
                      size_t taskIndex) noexcept;

  // Busy-waits until `pred` holds or the spin budget runs out. Returns what
  // `pred` last evaluated to, so a false result means the caller has to block.
  template <typename Pred>
  [[nodiscard]] bool SpinUntil(Pred pred) const noexcept
  {
    if (spinMicros == 0) {
      return pred();
    }

    // Reading the clock every iteration would cost more than the wait it is
    // bounding, so the deadline is only consulted once per batch of pauses.
    constexpr int kPausesPerClockCheck = 64;

    const auto deadline = std::chrono::steady_clock::now() +
      std::chrono::microseconds(spinMicros);

    for (;;) {
      for (int i = 0; i < kPausesPerClockCheck; ++i) {
        if (pred()) {
          return true;
        }
        CpuRelax();
      }
      if (std::chrono::steady_clock::now() >= deadline) {
        return pred();
      }
    }
  }

  static void CpuRelax() noexcept;

  [[nodiscard]] static uint32_t GenerationOf(uint64_t cursorValue) noexcept
  {
    return static_cast<uint32_t>(cursorValue >> 32);
  }

  [[nodiscard]] static uint32_t IndexOf(uint64_t cursorValue) noexcept
  {
    return static_cast<uint32_t>(cursorValue);
  }

  [[nodiscard]] static uint64_t MakeCursor(uint32_t generation,
                                           uint32_t index) noexcept
  {
    return (static_cast<uint64_t>(generation) << 32) | index;
  }

  std::vector<std::thread> workers;
  uint32_t spinMicros = kDefaultSpinMicros;

  std::mutex mutex;
  std::condition_variable cvWork;
  std::condition_variable cvDone;

  // Batch generation in the high 32 bits, next unclaimed task index in the
  // low 32. One word, so a worker can test "is this batch still mine" and
  // claim an index in a single compare-exchange.
  //
  // That pairing is what makes a straggler harmless. A worker left over from
  // the previous batch either fails the generation test and leaves, or fails
  // the exchange because the word moved under it and retries into the same
  // test. It can never consume an index belonging to a batch it is not part
  // of -- which is exactly what the earlier fetch_add cursor allowed, and the
  // failure was not a duplicated packet but a hung tick.
  std::atomic<uint64_t> cursor{ 0 };

  // Tasks finished in the current batch. Run waits on this rather than on
  // worker arrivals, so workers that never woke cost nothing.
  std::atomic<uint32_t> completedTasks{ 0 };
  std::atomic<uint32_t> taskCount{ 0 };

  // Read only after a worker has claimed an index, at which point Run is
  // provably still inside the call and the caller's vector is alive.
  //
  // Atomic because the accesses, while never actually concurrent, are not
  // ordered by anything a compiler or a race detector can see. The ordering
  // comes from the cursor: Run publishes this before the release store and
  // clears it only once every task is accounted for. Relaxed is therefore
  // enough on both sides.
  std::atomic<const std::vector<Task>*> currentTasks{ nullptr };

  // Bumped by Prime. Distinct from the cursor's generation so a woken worker
  // can tell "a batch exists" from "a batch is coming".
  std::atomic<uint64_t> primeToken{ 0 };

  std::atomic<bool> stopping{ false };

  // Workers currently inside cvWork.wait, and whether Run is inside
  // cvDone.wait. Both are guarded by `mutex`, and both exist so the common
  // case -- the workers already awake, nobody blocked -- can skip the notify
  // syscall entirely.
  size_t blockedWorkers = 0;
  bool runnerBlocked = false;

  // Workers not parked on cvWork: primed and spinning, or inside a batch.
  // Must be called with `mutex` held.
  [[nodiscard]] size_t WorkersAwake() const noexcept
  {
    return workers.size() - blockedWorkers;
  }

  std::atomic<uint64_t> failedTaskCount{ 0 };
};

}
