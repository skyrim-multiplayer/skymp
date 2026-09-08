#pragma once
#include "AreaCluster.h"
#include "AreaPartitioner.h"
#include "LoadBalancer.h"
#include "ParallelConfig.h"
#include "ParallelMetrics.h"
#include "RelayPlan.h"
#include "ThreadPool.h"
#include "TickSnapshot.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace MpParallel {

// Everything ActionListener knows about one movement update, flattened into
// plain data. The dispatcher copies what it needs, so nothing here has to
// outlive the call.
struct MovementSubmission
{
  uint32_t formId = 0;
  uint32_t idx = 0;
  Networking::UserId ownerUserId = Networking::InvalidUserId;

  float currentPos[3] = { 0.f, 0.f, 0.f };
  float currentRot[3] = { 0.f, 0.f, 0.f };
  uint32_t currentWorldOrCell = 0;

  float proposedPos[3] = { 0.f, 0.f, 0.f };
  float proposedRot[3] = { 0.f, 0.f, 0.f };
  uint32_t proposedWorldOrCell = 0;

  bool teleportFlag = false;
  bool isInJumpState = false;
  bool isWeapDrawn = false;
  bool isBlocking = false;
  bool isSneaking = false;
  bool isStanding = false;

  // The client's packet, forwarded to neighbours byte for byte.
  const uint8_t* packetData = nullptr;
  size_t packetLength = 0;
};

// The main-thread effects the join phase has to perform. Splitting them
// behind an interface is what lets the dispatcher be tested without a
// PartOne, an espm load order, or a socket.
//
// IMPORTANT for implementers: a submission and its join are separated by the
// rest of the packet pump, during which an actor can be destroyed or its
// owner can disconnect. Every method must re-resolve the actor by formId and
// do nothing if it is gone, and must check the user is still connected
// before sending.
class IOffloadSink
{
public:
  virtual ~IOffloadSink() = default;

  // Write the validated transform and animation state to the live actor.
  virtual void ApplyMovement(const ActorSnapshot& actor) = 0;

  // Pull the owning client back to the server's transform after a rejected
  // update.
  virtual void SendCorrection(const ActorSnapshot& actor) = 0;

  virtual void SendRelay(Networking::UserId userId, const uint8_t* data,
                         size_t length, bool reliable) = 0;
};

// Collects movement updates during packet ingest, processes them across the
// worker pool once per tick, and applies the results in a deterministic
// order.
//
// Threading contract: every public method is main-thread only and none may be
// called while ExecuteTick is running. Worker threads exist solely inside
// ExecuteTick and touch nothing but the snapshot (read) and the one
// ClusterOutput belonging to their work unit (write).
class OffloadDispatcher
{
public:
  explicit OffloadDispatcher(const ParallelConfig& config);
  ~OffloadDispatcher();

  OffloadDispatcher(const OffloadDispatcher&) = delete;
  OffloadDispatcher& operator=(const OffloadDispatcher&) = delete;

  // Rebuilds the pool if the worker count changed. Safe to call between
  // ticks; discards anything already submitted.
  void Reconfigure(const ParallelConfig& newConfig);

  [[nodiscard]] const ParallelConfig& GetConfig() const noexcept
  {
    return config;
  }

  [[nodiscard]] bool IsEnabled() const noexcept { return config.enabled; }

  // Returns false when the update was not taken on, in which case the caller
  // must fall back to handling it inline. That happens when the framework is
  // disabled and on any malformed submission, so a rejection is always safe.
  bool SubmitMovement(const MovementSubmission& submission);

  // Called once per tick by the main thread to snapshot all active listeners.
  // This $O(N)$ snapshot is passed to workers so they can spatially filter
  // recipients without enumerating the $O(N^2)$ edges on the main thread.
  void SetPotentialTargets(std::vector<RelayTarget>&& targets);

  [[nodiscard]] size_t GetPendingCount() const noexcept
  {
    return snapshot.actors.size();
  }

  // Runs the parallel phase and then the join. Clears the pending set.
  void ExecuteTick(IOffloadSink& sink);

  // Throws away pending work without applying it. Used when the world is
  // being torn down.
  void DiscardPending() noexcept;

  [[nodiscard]] const ParallelMetrics& GetMetrics() const noexcept
  {
    return metrics;
  }

  [[nodiscard]] const std::vector<AreaCluster>& GetLastClusters() const noexcept
  {
    return clusters;
  }

  // Exposed for tests that need to drive several ticks and observe the
  // throttle warming up.
  [[nodiscard]] uint64_t GetTickIndex() const noexcept
  {
    return snapshot.tickIndex;
  }

private:
  // A contiguous slice of one cluster's members. The unit of scheduling.
  //
  // Splitting clusters matters more than splitting across them: on a typical
  // populated server one hub holds the large majority of a tick's relay work,
  // so a cluster-sized task would leave most cores idle exactly when the
  // server is busiest.
  struct WorkUnit
  {
    uint32_t clusterIndex = 0;
    // Offset and length within AreaCluster::actorIndices.
    uint32_t begin = 0;
    uint32_t count = 0;
  };

  // allowSharding is false on the inline path, where splitting a cluster
  // would only add per-unit bookkeeping to work that runs serially anyway.
  void BuildWorkUnits(bool allowSharding);
  void RunUnits();
  void JoinResults(IOffloadSink& sink);
  void ResetPool();
  [[nodiscard]] size_t ComputeShardCount(size_t clusterSize) const;

  ParallelConfig config;
  std::unique_ptr<ThreadPool> pool;

  TickSnapshot snapshot;
  AreaPartitioner partitioner;
  LoadBalancer loadBalancer;

  std::vector<AreaCluster> clusters;
  std::vector<uint32_t> schedule;
  std::vector<uint32_t> pressureByCluster;

  // Parallel arrays indexed by work-unit index. Units are emitted in cluster
  // order, then in ascending member order inside a cluster, so walking them
  // in index order during the join reproduces the single-threaded sequence.
  std::vector<WorkUnit> workUnits;
  std::vector<ClusterOutput> unitOutputs;

  // Unit indices in scheduling order: busiest cluster's shards first. Only
  // affects which task enters the queue first, never the join order.
  std::vector<uint32_t> orderedUnitIndices;

  std::vector<ThreadPool::Task> tasks;
  std::vector<uint32_t> inlineUnitIndices;

  // Scratch for aggregating shard times back to their cluster.
  std::vector<uint64_t> clusterMicros;

  // Scratch: position of each cluster in `schedule`, so units can be ordered
  // by their cluster's rank without a lookup per comparison.
  std::vector<uint32_t> clusterRank;

  ParallelMetrics metrics;
  uint64_t lastFailedTaskCount = 0;
};

}
