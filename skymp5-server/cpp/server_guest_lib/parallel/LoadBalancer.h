#pragma once
#include "AreaCluster.h"
#include "AreaKey.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace MpParallel {

// Decides the order clusters are handed to the pool in, and how hard each one
// should throttle.
//
// Cluster costs differ by an order of magnitude between a market square and a
// lone traveller on a road, and the cost of a given area is strongly
// correlated tick to tick. Scheduling the expensive ones first (longest
// processing time first) is the standard greedy answer to that: it keeps the
// long pole from being picked up last and leaving every other slot waiting at
// the barrier.
class LoadBalancer
{
public:
  // Records how long a cluster actually took, keyed by its stable identity.
  void Observe(const AreaKey& key, uint64_t elapsedMicros, uint64_t tickIndex);

  // Predicted cost. Falls back to a relay-edge proxy for an area that has not
  // been measured yet, which is what makes the very first tick in a new city
  // schedule sensibly rather than at random.
  [[nodiscard]] uint64_t EstimateMicros(const AreaCluster& cluster) const;

  // 0 means "within budget, do not throttle". Higher values ask the interest
  // manager to space distant relays further apart.
  [[nodiscard]] uint32_t GetPressureLevel(const AreaKey& key,
                                          uint64_t perClusterBudgetMicros,
                                          uint32_t maxLevel) const;

  // Fills outOrder with cluster indices, most expensive first.
  void BuildSchedule(const std::vector<AreaCluster>& clusters,
                     std::vector<uint32_t>& outOrder) const;

  // Drops areas nobody has visited for a while so the table does not grow
  // without bound as players roam the map.
  void EvictStale(uint64_t currentTickIndex, uint64_t maxAgeTicks);

  [[nodiscard]] size_t GetTrackedAreaCount() const noexcept
  {
    return costByArea.size();
  }

  void Clear() noexcept { costByArea.clear(); }

private:
  struct CostEntry
  {
    // Exponentially weighted moving average of measured microseconds.
    double emaMicros = 0.0;
    uint64_t lastMicros = 0;
    uint64_t lastSeenTick = 0;
    uint32_t sampleCount = 0;
  };

  // Weight of the newest sample. High enough to react to a crowd forming
  // within a couple of ticks, low enough that one slow tick caused by an
  // unrelated stall does not pin the area at maximum throttle.
  static constexpr double kEmaAlpha = 0.3;

  // Microseconds attributed to one actor before any measurement exists. Only
  // used for cold-start ordering, so being off by a factor of two costs
  // nothing but a slightly worse schedule on the first tick.
  //
  // There is no per-edge term: relay edges are no longer enumerated on the
  // main thread, so a cluster's cost is estimated from its population.
  static constexpr double kColdStartMicrosPerActor = 2.0;

  std::unordered_map<AreaKey, CostEntry, AreaKeyHash> costByArea;
};

}
