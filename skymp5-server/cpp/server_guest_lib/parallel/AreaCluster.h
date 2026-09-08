#pragma once
#include "AreaKey.h"
#include <cstdint>
#include <vector>

namespace MpParallel {

// A set of actors that can be processed independently of every other
// cluster in the same tick.
struct AreaCluster
{
  // Smallest AreaKey in the cluster. Doubles as the cluster's identity for
  // cost tracking across ticks and as its sort key, which is what makes the
  // join order reproducible.
  AreaKey representative;

  // Indices into TickSnapshot::actors, ascending.
  std::vector<uint32_t> actorIndices;

  // Number of chunks occupied by this cluster. Cheap density signal for the
  // cost model.
  uint32_t chunkCount = 0;

  [[nodiscard]] size_t Size() const noexcept { return actorIndices.size(); }
};

}
