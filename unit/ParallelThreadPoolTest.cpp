#include "parallel/ThreadPool.h"
#include <atomic>
#include <catch2/catch_all.hpp>
#include <chrono>
#include <mutex>
#include <numeric>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

using MpParallel::ThreadPool;

TEST_CASE("Zero workers still runs every task on the caller", "[ParallelPool]")
{
  ThreadPool pool(0);
  REQUIRE(pool.GetWorkerCount() == 0);
  REQUIRE(pool.GetSlotCount() == 1);

  std::vector<int> ran(16, 0);
  std::vector<ThreadPool::Task> tasks;
  for (size_t i = 0; i < ran.size(); ++i) {
    tasks.emplace_back([&ran, i](size_t slot) {
      REQUIRE(slot == 0);
      ran[i] = 1;
    });
  }

  pool.Run(tasks);

  REQUIRE(std::accumulate(ran.begin(), ran.end(), 0) ==
          static_cast<int>(ran.size()));
}

TEST_CASE("Every task runs exactly once", "[ParallelPool]")
{
  ThreadPool pool(4);

  constexpr size_t kTaskCount = 500;
  std::vector<std::atomic<int>> counters(kTaskCount);
  for (auto& counter : counters) {
    counter.store(0);
  }

  std::vector<ThreadPool::Task> tasks;
  tasks.reserve(kTaskCount);
  for (size_t i = 0; i < kTaskCount; ++i) {
    tasks.emplace_back(
      [&counters, i](size_t) { counters[i].fetch_add(1); });
  }

  pool.Run(tasks);

  for (size_t i = 0; i < kTaskCount; ++i) {
    REQUIRE(counters[i].load() == 1);
  }
}

TEST_CASE("Slot indices stay inside the advertised range", "[ParallelPool]")
{
  ThreadPool pool(3);
  REQUIRE(pool.GetSlotCount() == 4);

  std::mutex mutex;
  std::set<size_t> seenSlots;

  std::vector<ThreadPool::Task> tasks;
  for (size_t i = 0; i < 400; ++i) {
    tasks.emplace_back([&](size_t slot) {
      REQUIRE(slot < 4);
      std::lock_guard<std::mutex> lock(mutex);
      seenSlots.insert(slot);
    });
  }

  pool.Run(tasks);

  // The caller always participates, so slot 0 must show up.
  REQUIRE(seenSlots.count(0) == 1);
}

TEST_CASE("Repeated batches all complete", "[ParallelPool]")
{
  ThreadPool pool(4);
  std::atomic<int> total{ 0 };

  for (int round = 0; round < 50; ++round) {
    std::vector<ThreadPool::Task> tasks;
    for (int i = 0; i < 20; ++i) {
      tasks.emplace_back([&total](size_t) { total.fetch_add(1); });
    }
    pool.Run(tasks);
    // The barrier must have drained before Run returns.
    REQUIRE(total.load() == (round + 1) * 20);
  }
}

TEST_CASE("Back-to-back batches never bleed into each other",
          "[ParallelPool]")
{
  // Regression: Run used to return as soon as tasksRemaining hit zero, while
  // the worker that ran the final task was still looping on the cursor. The
  // next batch reset that cursor, so the straggler could re-run a task from
  // the previous vector and decrement the new batch's counter. Each round
  // here uses a fresh vector of fresh closures, so a straggler that reached
  // back into the old batch shows up as a wrong per-round total.
  ThreadPool pool(4);

  for (int round = 0; round < 400; ++round) {
    std::atomic<int> ran{ 0 };
    std::vector<ThreadPool::Task> tasks;
    // Deliberately tiny: the shorter the batch, the likelier a worker is
    // still in the drain loop when Run wants to return.
    for (int i = 0; i < 3; ++i) {
      tasks.emplace_back([&ran](size_t) { ran.fetch_add(1); });
    }
    pool.Run(tasks);
    REQUIRE(ran.load() == 3);
  }
}

TEST_CASE("Alternating batch sizes stay consistent", "[ParallelPool]")
{
  // A long batch followed by a very short one is the shape most likely to
  // leave a worker draining across the boundary.
  ThreadPool pool(4);

  for (int round = 0; round < 120; ++round) {
    const int count = (round % 2 == 0) ? 200 : 1;
    std::atomic<int> ran{ 0 };
    std::vector<ThreadPool::Task> tasks;
    for (int i = 0; i < count; ++i) {
      tasks.emplace_back([&ran](size_t) { ran.fetch_add(1); });
    }
    pool.Run(tasks);
    REQUIRE(ran.load() == count);
  }
}

TEST_CASE("A throwing task does not wedge the barrier", "[ParallelPool]")
{
  ThreadPool pool(3);

  std::atomic<int> completed{ 0 };
  std::vector<ThreadPool::Task> tasks;
  for (int i = 0; i < 60; ++i) {
    tasks.emplace_back([&completed, i](size_t) {
      if (i % 10 == 0) {
        throw std::runtime_error("boom");
      }
      completed.fetch_add(1);
    });
  }

  // Would hang rather than fail if the failure path skipped its decrement.
  pool.Run(tasks);

  REQUIRE(completed.load() == 54);
  REQUIRE(pool.GetFailedTaskCount() == 6);

  // The pool stays usable afterwards.
  std::atomic<int> after{ 0 };
  std::vector<ThreadPool::Task> more;
  for (int i = 0; i < 10; ++i) {
    more.emplace_back([&after](size_t) { after.fetch_add(1); });
  }
  pool.Run(more);
  REQUIRE(after.load() == 10);
}

TEST_CASE("Empty batch is a no-op", "[ParallelPool]")
{
  ThreadPool pool(2);
  std::vector<ThreadPool::Task> tasks;
  pool.Run(tasks);
  REQUIRE(pool.GetFailedTaskCount() == 0);
}

TEST_CASE("Uneven task costs still drain", "[ParallelPool]")
{
  // Dynamic scheduling exists so that one long task cannot leave the other
  // slots idle. This checks the barrier copes with a lopsided batch.
  ThreadPool pool(4);

  std::atomic<int> done{ 0 };
  std::vector<ThreadPool::Task> tasks;
  tasks.emplace_back([&done](size_t) {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    done.fetch_add(1);
  });
  for (int i = 0; i < 100; ++i) {
    tasks.emplace_back([&done](size_t) { done.fetch_add(1); });
  }

  pool.Run(tasks);
  REQUIRE(done.load() == 101);
}
