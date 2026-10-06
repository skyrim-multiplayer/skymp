#pragma once
#include "AreaCluster.h"
#include "AreaKey.h"
#include "TickSnapshot.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace MpParallel {

// Groups the actors submitted this tick into independently processable
// clusters.
//
// WHY A SEPARATION PARAMETER
//
// The server's visibility model is the 3x3 chunk stencil in Grid.h: an actor
// standing in chunk C is subscribed to, and relays to, exactly the actors
// occupying chunks within Chebyshev distance 1 of C. Movement is capped by
// MovementValidation at just under one chunk per update, so within a single
// tick an actor's relay set can reach chunks up to Chebyshev distance 2 from
// where it started.
//
// A cluster is therefore only self-contained if every actor it can relay to
// is also inside it. Building clusters as connected components under the
// relation "same worldOrCell and Chebyshev chunk distance <= S" achieves
// that for any S >= 3, which is why ParallelConfig clamps the setting up to
// kMinSafeSeparationChunks. Larger values trade parallelism for margin: they
// merge nearby crowds into one cluster rather than risking a split through
// the middle of a group that can see each other.
//
// Two actors in different worldspaces or interior cells never share a grid at
// all, so those always separate.
//
// DETERMINISM
//
// Clusters come back ordered by their smallest AreaKey, and each cluster's
// member list is ascending. The join phase walks them in that order, so the
// order in which state is applied does not depend on how many worker threads
// happen to be available.
class AreaPartitioner
{
public:
  // Assigns every actor a clusterIndex and fills outClusters.
  //
  // Actors are partitioned by their `area` field, which callers set from the
  // actor's server-authoritative position, not the position the client is
  // proposing. Using the authoritative one keeps an actor from hopping
  // clusters because of a packet that is about to be rejected.
  void Partition(std::vector<ActorSnapshot>& actors, int32_t separationChunks,
                 std::vector<AreaCluster>& outClusters);

  // Number of distinct occupied chunks seen by the most recent Partition
  // call. Exposed for metrics and tests.
  [[nodiscard]] size_t GetLastChunkCount() const noexcept
  {
    return chunkKeys.size();
  }

private:
  uint32_t Find(uint32_t node);
  void Unite(uint32_t a, uint32_t b);

  // Scratch reused across ticks so a steady-state tick allocates nothing.
  std::unordered_map<AreaKey, uint32_t, AreaKeyHash> chunkIndexByKey;
  std::vector<AreaKey> chunkKeys;
  std::vector<uint32_t> parent;
  std::vector<uint32_t> unionRank;
  std::vector<uint32_t> clusterOfRoot;
  std::vector<uint32_t> clusterOfChunk;

  // Chunk each actor occupies, filled while the distinct chunks are being
  // discovered. Keeping it means the actor-to-cluster pass is a pair of array
  // reads instead of a hash lookup per actor, which matters because that pass
  // is the only one that is O(actors) rather than O(occupied chunks).
  std::vector<uint32_t> actorChunkSlot;

  // Permutation that takes first-seen chunk order to sorted order, and the
  // buffer the sorted keys are built into.
  std::vector<uint32_t> chunkOrder;
  std::vector<uint32_t> chunkRemap;
  std::vector<AreaKey> sortedChunkKeys;
};

}
