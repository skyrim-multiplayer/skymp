#include "AreaPartitioner.h"

#include <algorithm>
#include <limits>

namespace MpParallel {

namespace {

constexpr int32_t kInt16Min = std::numeric_limits<int16_t>::min();
constexpr int32_t kInt16Max = std::numeric_limits<int16_t>::max();

}

uint32_t AreaPartitioner::Find(uint32_t node)
{
  // Iterative path halving: no recursion, so a pathological chain cannot
  // overflow the stack on a worker with a small default stack size.
  while (parent[node] != node) {
    parent[node] = parent[parent[node]];
    node = parent[node];
  }
  return node;
}

void AreaPartitioner::Unite(uint32_t a, uint32_t b)
{
  uint32_t rootA = Find(a);
  uint32_t rootB = Find(b);
  if (rootA == rootB) {
    return;
  }

  if (unionRank[rootA] < unionRank[rootB]) {
    std::swap(rootA, rootB);
  }
  parent[rootB] = rootA;
  if (unionRank[rootA] == unionRank[rootB]) {
    ++unionRank[rootA];
  }
}

void AreaPartitioner::Partition(std::vector<ActorSnapshot>& actors,
                                int32_t separationChunks,
                                std::vector<AreaCluster>& outClusters)
{
  outClusters.clear();
  chunkIndexByKey.clear();
  chunkKeys.clear();
  parent.clear();
  unionRank.clear();
  clusterOfRoot.clear();
  clusterOfChunk.clear();

  if (actors.empty()) {
    return;
  }

  const int32_t separation =
    std::max(separationChunks, kMinSafeSeparationChunks);

  // Pass 1: collect the distinct occupied chunks.
  //
  // Deduplicating through the hash map rather than by sorting the whole actor
  // list keeps this O(actors) instead of O(actors log actors). The difference
  // is the normal case, not the exotic one: a crowd is many actors in few
  // chunks, so sorting one key per actor was sorting the same value over and
  // over.
  actorChunkSlot.resize(actors.size());
  for (uint32_t i = 0; i < static_cast<uint32_t>(actors.size()); ++i) {
    const auto inserted = chunkIndexByKey.emplace(
      actors[i].area, static_cast<uint32_t>(chunkKeys.size()));
    if (inserted.second) {
      chunkKeys.push_back(actors[i].area);
    }
    actorChunkSlot[i] = inserted.first->second;
  }

  const uint32_t chunkCount = static_cast<uint32_t>(chunkKeys.size());

  // chunkKeys is in first-seen order, which depends on the order packets
  // happened to arrive in. Renumbering the chunks into sorted order here is
  // what makes everything downstream -- cluster indices, member lists, the
  // join sequence -- reproducible.
  chunkOrder.resize(chunkCount);
  for (uint32_t i = 0; i < chunkCount; ++i) {
    chunkOrder[i] = i;
  }
  std::sort(chunkOrder.begin(), chunkOrder.end(),
            [this](uint32_t lhs, uint32_t rhs) {
              return chunkKeys[lhs] < chunkKeys[rhs];
            });

  chunkRemap.resize(chunkCount);
  sortedChunkKeys.resize(chunkCount);
  for (uint32_t rank = 0; rank < chunkCount; ++rank) {
    chunkRemap[chunkOrder[rank]] = rank;
    sortedChunkKeys[rank] = chunkKeys[chunkOrder[rank]];
  }
  chunkKeys.swap(sortedChunkKeys);

  for (auto& entry : chunkIndexByKey) {
    entry.second = chunkRemap[entry.second];
  }
  for (uint32_t& slot : actorChunkSlot) {
    slot = chunkRemap[slot];
  }

  // Pass 2: union chunks that are within `separation` of each other. Probing
  // the (2S+1)^2 window around each chunk costs a constant per chunk, which
  // beats comparing every pair once more than a handful of chunks are
  // occupied.
  parent.resize(chunkCount);
  unionRank.assign(chunkCount, 0);
  for (uint32_t i = 0; i < chunkCount; ++i) {
    parent[i] = i;
  }

  for (uint32_t i = 0; i < chunkCount; ++i) {
    const AreaKey& key = chunkKeys[i];
    for (int32_t dx = -separation; dx <= separation; ++dx) {
      const int32_t nx = static_cast<int32_t>(key.chunkX) + dx;
      if (nx < kInt16Min || nx > kInt16Max) {
        continue;
      }
      for (int32_t dy = -separation; dy <= separation; ++dy) {
        if (dx == 0 && dy == 0) {
          continue;
        }
        const int32_t ny = static_cast<int32_t>(key.chunkY) + dy;
        if (ny < kInt16Min || ny > kInt16Max) {
          continue;
        }

        const AreaKey probe{ key.worldOrCell, static_cast<int16_t>(nx),
                             static_cast<int16_t>(ny) };
        auto it = chunkIndexByKey.find(probe);
        if (it != chunkIndexByKey.end()) {
          Unite(i, it->second);
        }
      }
    }
  }

  // Pass 3: turn union-find roots into cluster indices. Scanning the sorted
  // chunk list and numbering roots on first sight orders clusters by their
  // smallest AreaKey.
  clusterOfChunk.assign(chunkCount, kUnassignedCluster);
  clusterOfRoot.assign(chunkCount, kUnassignedCluster);

  uint32_t nextClusterIndex = 0;
  for (uint32_t i = 0; i < chunkCount; ++i) {
    const uint32_t root = Find(i);
    if (clusterOfRoot[root] == kUnassignedCluster) {
      clusterOfRoot[root] = nextClusterIndex++;
    }
    clusterOfChunk[i] = clusterOfRoot[root];
  }

  outClusters.resize(nextClusterIndex);
  for (uint32_t i = 0; i < chunkCount; ++i) {
    AreaCluster& cluster = outClusters[clusterOfChunk[i]];
    if (cluster.chunkCount == 0) {
      // chunkKeys is sorted, so the first chunk seen for a cluster is its
      // smallest and therefore its stable identity.
      cluster.representative = chunkKeys[i];
    }
    ++cluster.chunkCount;
  }

  // Pass 4: hand the actors out. Iterating actors in index order keeps every
  // actorIndices list ascending without a second sort.
  for (uint32_t actorIndex = 0;
       actorIndex < static_cast<uint32_t>(actors.size()); ++actorIndex) {
    const uint32_t clusterIndex = clusterOfChunk[actorChunkSlot[actorIndex]];
    actors[actorIndex].clusterIndex = clusterIndex;
    outClusters[clusterIndex].actorIndices.push_back(actorIndex);
  }
}

}
