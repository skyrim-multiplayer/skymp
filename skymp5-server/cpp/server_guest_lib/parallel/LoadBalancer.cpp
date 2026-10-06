#include "LoadBalancer.h"

#include <algorithm>

namespace MpParallel {

void LoadBalancer::Observe(const AreaKey& key, uint64_t elapsedMicros,
                           uint64_t tickIndex)
{
  CostEntry& entry = costByArea[key];
  const double sample = static_cast<double>(elapsedMicros);

  if (entry.sampleCount == 0) {
    entry.emaMicros = sample;
  } else {
    entry.emaMicros = kEmaAlpha * sample + (1.0 - kEmaAlpha) * entry.emaMicros;
  }

  entry.lastMicros = elapsedMicros;
  entry.lastSeenTick = tickIndex;
  if (entry.sampleCount < UINT32_MAX) {
    ++entry.sampleCount;
  }
}

uint64_t LoadBalancer::EstimateMicros(const AreaCluster& cluster) const
{
  auto it = costByArea.find(cluster.representative);
  if (it != costByArea.end() && it->second.sampleCount > 0) {
    return static_cast<uint64_t>(it->second.emaMicros);
  }

  const double estimate =
    static_cast<double>(cluster.Size()) * kColdStartMicrosPerActor;
  return static_cast<uint64_t>(estimate);
}

uint32_t LoadBalancer::GetPressureLevel(const AreaKey& key,
                                        uint64_t perClusterBudgetMicros,
                                        uint32_t maxLevel) const
{
  if (perClusterBudgetMicros == 0 || maxLevel == 0) {
    return 0;
  }

  auto it = costByArea.find(key);
  if (it == costByArea.end() || it->second.sampleCount == 0) {
    return 0;
  }

  // Judge on the smoothed cost rather than the last raw sample: a single
  // slow tick, e.g. one that collided with a garbage collection pause, is
  // not a reason to start dropping updates on players.
  const double smoothed = it->second.emaMicros;
  const double budget = static_cast<double>(perClusterBudgetMicros);
  if (smoothed <= budget) {
    return 0;
  }

  const uint64_t ratio = static_cast<uint64_t>(smoothed / budget);
  return static_cast<uint32_t>(std::min<uint64_t>(ratio, maxLevel));
}

void LoadBalancer::BuildSchedule(const std::vector<AreaCluster>& clusters,
                                 std::vector<uint32_t>& outOrder) const
{
  outOrder.clear();
  outOrder.reserve(clusters.size());
  for (uint32_t i = 0; i < static_cast<uint32_t>(clusters.size()); ++i) {
    outOrder.push_back(i);
  }

  std::stable_sort(outOrder.begin(), outOrder.end(),
                   [this, &clusters](uint32_t lhs, uint32_t rhs) {
                     const uint64_t lhsCost = EstimateMicros(clusters[lhs]);
                     const uint64_t rhsCost = EstimateMicros(clusters[rhs]);
                     return lhsCost > rhsCost;
                   });
}

void LoadBalancer::EvictStale(uint64_t currentTickIndex, uint64_t maxAgeTicks)
{
  if (maxAgeTicks == 0 || currentTickIndex < maxAgeTicks) {
    return;
  }

  const uint64_t cutoff = currentTickIndex - maxAgeTicks;
  for (auto it = costByArea.begin(); it != costByArea.end();) {
    if (it->second.lastSeenTick < cutoff) {
      it = costByArea.erase(it);
    } else {
      ++it;
    }
  }
}

}
