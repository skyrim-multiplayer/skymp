#pragma once
#include <cstddef>
#include <cstdint>

namespace MpParallel {

// Counters for the most recent tick plus running totals. Written only by the
// main thread during the join, so no synchronisation is needed.
struct ParallelMetrics
{
  // --- most recent tick -------------------------------------------------
  uint64_t lastTickIndex = 0;
  size_t lastActorCount = 0;
  size_t lastClusterCount = 0;
  size_t lastChunkCount = 0;

  // Total scheduling units this tick, and how many of them went to the pool.
  // A unit is a slice of a cluster, so this exceeds the cluster count
  // whenever a busy area was split across cores.
  size_t lastWorkUnitCount = 0;
  size_t lastPooledUnitCount = 0;

  // Members of the largest single cluster. Watch this against
  // lastActorCount: if one cluster holds most of the population, sharding is
  // what is buying the speedup.
  size_t lastLargestClusterSize = 0;

  uint64_t lastRelayEdgesEmitted = 0;
  uint64_t lastRelayEdgesThrottled = 0;
  uint64_t lastRejectedMovements = 0;

  // Wall clock of the fork/join phase itself.
  uint64_t lastParallelMicros = 0;
  // Wall clock of the serial join that follows it.
  uint64_t lastJoinMicros = 0;
  // Sum of the per-cluster task times. Divided by lastParallelMicros this
  // gives the achieved speedup, which is the number worth watching.
  uint64_t lastAggregateTaskMicros = 0;

  // --- running totals ---------------------------------------------------
  uint64_t totalTicks = 0;
  uint64_t totalOffloadedTicks = 0;
  uint64_t totalInlineTicks = 0;
  uint64_t totalRelayEdgesEmitted = 0;
  uint64_t totalRelayEdgesThrottled = 0;
  uint64_t totalFailedTasks = 0;

  // Ratio of aggregate task time to wall-clock parallel time. 1.0 means the
  // offload bought nothing; the theoretical ceiling is the slot count.
  [[nodiscard]] double GetLastSpeedup() const noexcept
  {
    if (lastParallelMicros == 0) {
      return 0.0;
    }
    return static_cast<double>(lastAggregateTaskMicros) /
      static_cast<double>(lastParallelMicros);
  }

  [[nodiscard]] double GetLastThrottleRatio() const noexcept
  {
    const uint64_t total = lastRelayEdgesEmitted + lastRelayEdgesThrottled;
    if (total == 0) {
      return 0.0;
    }
    return static_cast<double>(lastRelayEdgesThrottled) /
      static_cast<double>(total);
  }

  void ResetTick() noexcept
  {
    lastActorCount = 0;
    lastClusterCount = 0;
    lastChunkCount = 0;
    lastWorkUnitCount = 0;
    lastPooledUnitCount = 0;
    lastLargestClusterSize = 0;
    lastRelayEdgesEmitted = 0;
    lastRelayEdgesThrottled = 0;
    lastRejectedMovements = 0;
    lastParallelMicros = 0;
    lastJoinMicros = 0;
    lastAggregateTaskMicros = 0;
  }
};

}
