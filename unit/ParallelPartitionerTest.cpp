#include "parallel/AreaPartitioner.h"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <set>
#include <vector>

using namespace MpParallel;

namespace {

ActorSnapshot MakeActor(uint32_t formId, uint32_t worldOrCell, int16_t chunkX,
                        int16_t chunkY)
{
  ActorSnapshot actor;
  actor.formId = formId;
  actor.worldOrCell = worldOrCell;
  actor.area = AreaKey{ worldOrCell, chunkX, chunkY };
  return actor;
}

// Every pair of actors placed in different clusters must be far enough apart
// that neither can appear in the other's relay set.
void RequireClustersAreIndependent(const std::vector<ActorSnapshot>& actors,
                                   int32_t separation)
{
  for (size_t i = 0; i < actors.size(); ++i) {
    for (size_t j = i + 1; j < actors.size(); ++j) {
      if (actors[i].clusterIndex == actors[j].clusterIndex) {
        continue;
      }
      const int32_t distance = actors[i].area.ChunkDistanceTo(actors[j].area);
      const bool differentWorlds = distance < 0;
      REQUIRE((differentWorlds || distance > separation));
    }
  }
}

}

TEST_CASE("Empty input yields no clusters", "[ParallelPartition]")
{
  AreaPartitioner partitioner;
  std::vector<ActorSnapshot> actors;
  std::vector<AreaCluster> clusters;

  partitioner.Partition(actors, 4, clusters);

  REQUIRE(clusters.empty());
  REQUIRE(partitioner.GetLastChunkCount() == 0);
}

TEST_CASE("Actors in the same chunk share a cluster", "[ParallelPartition]")
{
  AreaPartitioner partitioner;
  std::vector<ActorSnapshot> actors{
    MakeActor(1, 0x3c, 10, 10),
    MakeActor(2, 0x3c, 10, 10),
    MakeActor(3, 0x3c, 10, 10),
  };
  std::vector<AreaCluster> clusters;

  partitioner.Partition(actors, 4, clusters);

  REQUIRE(clusters.size() == 1);
  REQUIRE(clusters[0].Size() == 3);
  REQUIRE(partitioner.GetLastChunkCount() == 1);
  REQUIRE(actors[0].clusterIndex == actors[1].clusterIndex);
  REQUIRE(actors[1].clusterIndex == actors[2].clusterIndex);
}

TEST_CASE("Distant groups split, close ones do not", "[ParallelPartition]")
{
  AreaPartitioner partitioner;
  std::vector<AreaCluster> clusters;

  SECTION("Within the separation distance they merge")
  {
    // Chebyshev distance 3 with separation 4: still one cluster.
    std::vector<ActorSnapshot> actors{
      MakeActor(1, 0x3c, 0, 0),
      MakeActor(2, 0x3c, 3, 0),
    };
    partitioner.Partition(actors, 4, clusters);
    REQUIRE(clusters.size() == 1);
  }

  SECTION("Beyond the separation distance they split")
  {
    // Chebyshev distance 5 with separation 4: two clusters.
    std::vector<ActorSnapshot> actors{
      MakeActor(1, 0x3c, 0, 0),
      MakeActor(2, 0x3c, 5, 0),
    };
    partitioner.Partition(actors, 4, clusters);
    REQUIRE(clusters.size() == 2);
    REQUIRE(actors[0].clusterIndex != actors[1].clusterIndex);
    RequireClustersAreIndependent(actors, 4);
  }
}

TEST_CASE("Chains of nearby actors stay in one cluster",
          "[ParallelPartition]")
{
  // Each hop is within the separation, so transitivity has to pull the whole
  // line together even though the ends are 20 chunks apart.
  AreaPartitioner partitioner;
  std::vector<ActorSnapshot> actors;
  for (int16_t i = 0; i < 10; ++i) {
    actors.push_back(MakeActor(static_cast<uint32_t>(i + 1), 0x3c,
                               static_cast<int16_t>(i * 2), 0));
  }

  std::vector<AreaCluster> clusters;
  partitioner.Partition(actors, 3, clusters);

  REQUIRE(clusters.size() == 1);
  REQUIRE(clusters[0].Size() == 10);
  REQUIRE(clusters[0].chunkCount == 10);
}

TEST_CASE("Different worldspaces never merge", "[ParallelPartition]")
{
  AreaPartitioner partitioner;
  std::vector<ActorSnapshot> actors{
    MakeActor(1, 0x3c, 0, 0),
    MakeActor(2, 0x1a26f, 0, 0), // identical chunk, different cell
  };
  std::vector<AreaCluster> clusters;

  partitioner.Partition(actors, 64, clusters);

  REQUIRE(clusters.size() == 2);
  REQUIRE(actors[0].clusterIndex != actors[1].clusterIndex);
}

TEST_CASE("Separation is clamped to the safe minimum", "[ParallelPartition]")
{
  // Asking for 0 would put neighbouring chunks in different clusters and
  // break the relay-locality guarantee, so the partitioner must ignore it.
  AreaPartitioner partitioner;
  std::vector<ActorSnapshot> actors{
    MakeActor(1, 0x3c, 0, 0),
    MakeActor(2, 0x3c, 2, 0),
  };
  std::vector<AreaCluster> clusters;

  partitioner.Partition(actors, 0, clusters);

  REQUIRE(clusters.size() == 1);
}

TEST_CASE("Cluster order does not depend on submission order",
          "[ParallelPartition]")
{
  AreaPartitioner partitioner;
  std::vector<AreaCluster> forward;
  std::vector<AreaCluster> reversed;

  std::vector<ActorSnapshot> actorsForward{
    MakeActor(1, 0x3c, 0, 0),   MakeActor(2, 0x3c, 40, 40),
    MakeActor(3, 0x3c, 80, 80), MakeActor(4, 0x3c, 1, 1),
  };
  std::vector<ActorSnapshot> actorsReversed{
    MakeActor(4, 0x3c, 1, 1),   MakeActor(3, 0x3c, 80, 80),
    MakeActor(2, 0x3c, 40, 40), MakeActor(1, 0x3c, 0, 0),
  };

  partitioner.Partition(actorsForward, 4, forward);
  partitioner.Partition(actorsReversed, 4, reversed);

  REQUIRE(forward.size() == reversed.size());
  for (size_t i = 0; i < forward.size(); ++i) {
    REQUIRE(forward[i].representative == reversed[i].representative);
  }
}

TEST_CASE("Member lists are ascending and cover every actor",
          "[ParallelPartition]")
{
  AreaPartitioner partitioner;
  std::vector<ActorSnapshot> actors;
  for (int16_t i = 0; i < 40; ++i) {
    actors.push_back(MakeActor(static_cast<uint32_t>(i + 1), 0x3c,
                               static_cast<int16_t>((i % 5) * 30),
                               static_cast<int16_t>(i / 5)));
  }

  std::vector<AreaCluster> clusters;
  partitioner.Partition(actors, 4, clusters);

  size_t covered = 0;
  std::set<uint32_t> seen;
  for (const AreaCluster& cluster : clusters) {
    REQUIRE(std::is_sorted(cluster.actorIndices.begin(),
                           cluster.actorIndices.end()));
    covered += cluster.Size();
    for (uint32_t actorIndex : cluster.actorIndices) {
      REQUIRE(seen.insert(actorIndex).second);
      REQUIRE(actors[actorIndex].clusterIndex ==
              static_cast<uint32_t>(&cluster - clusters.data()));
    }
  }

  REQUIRE(covered == actors.size());
  RequireClustersAreIndependent(actors, 4);
}

TEST_CASE("Partitioner scratch is reusable across calls",
          "[ParallelPartition]")
{
  AreaPartitioner partitioner;
  std::vector<AreaCluster> clusters;

  std::vector<ActorSnapshot> first{ MakeActor(1, 0x3c, 0, 0),
                                    MakeActor(2, 0x3c, 50, 50) };
  partitioner.Partition(first, 4, clusters);
  REQUIRE(clusters.size() == 2);

  std::vector<ActorSnapshot> second{ MakeActor(3, 0x3c, 7, 7) };
  partitioner.Partition(second, 4, clusters);
  REQUIRE(clusters.size() == 1);
  REQUIRE(clusters[0].Size() == 1);
  REQUIRE(partitioner.GetLastChunkCount() == 1);
}

TEST_CASE("Chunk coordinates match the server's grid math",
          "[ParallelPartition]")
{
  // GetGridPos in MpObjectReference.cpp truncates towards zero. Reproducing
  // that exactly is what keeps clusters aligned with subscription
  // neighbourhoods.
  REQUIRE(ToChunkCoord(0.f) == 0);
  REQUIRE(ToChunkCoord(4095.f) == 0);
  REQUIRE(ToChunkCoord(4096.f) == 1);
  REQUIRE(ToChunkCoord(-1.f) == 0);
  REQUIRE(ToChunkCoord(-4096.f) == -1);
  REQUIRE(ToChunkCoord(8192.f) == 2);
}

TEST_CASE("AreaKey distance and hashing behave", "[ParallelPartition]")
{
  const AreaKey a{ 0x3c, 0, 0 };
  const AreaKey b{ 0x3c, 3, -4 };
  const AreaKey elsewhere{ 0x40, 0, 0 };

  REQUIRE(a.ChunkDistanceTo(b) == 4);
  REQUIRE(b.ChunkDistanceTo(a) == 4);
  REQUIRE(a.ChunkDistanceTo(elsewhere) == -1);
  REQUIRE(a.ChunkDistanceTo(a) == 0);

  AreaKeyHash hash;
  REQUIRE(hash(a) == hash(AreaKey{ 0x3c, 0, 0 }));
  REQUIRE(hash(a) != hash(b));

  // Negative coordinates must not collide with their positive twins.
  REQUIRE(hash(AreaKey{ 0x3c, -1, -1 }) != hash(AreaKey{ 0x3c, 1, 1 }));
}
