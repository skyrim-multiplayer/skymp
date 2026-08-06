#pragma once
#include "AreaCluster.h"
#include "AreaPartitioner.h"
#include "InterestManager.h"
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

  // Hands over one work unit's relay list in one call.
  //
  // Per-edge rather than per-batch was the obvious shape and the wrong one:
  // relaying is the N^2 term, so at 400 players in one area this was 160,000
  // virtual calls a tick, each of which then re-resolved the send target --
  // a pointer chase and a throw-if-null -- and re-checked the recipient's
  // connection. All three are loop invariants, and a batched call is what
  // lets the implementation hoist them.
  //
  // `sends` index into [packetBytes, packetBytes + packetBytesLength). The
  // implementation must bounds-check them; a range that does not fit is a
  // malformed submission and should be skipped, not clamped.
  virtual void SendRelayBatch(const OutboundSend* sends, size_t count,
                              const uint8_t* packetBytes,
                              size_t packetBytesLength) = 0;
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

  // Allocation-free form of the above: the caller fills the returned (empty)
  // buffer in place and then calls CommitPotentialTargets. Rebuilding the
  // list is a per-tick job, so handing over a fresh vector every time meant a
  // heap round trip per tick for no reason.
  [[nodiscard]] std::vector<RelayTarget>& BeginPotentialTargets() noexcept;
  void CommitPotentialTargets();

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
  [[nodiscard]] size_t ComputeShardCount(size_t clusterSize,
                                         size_t shardBudget) const;

  // How many shards this tick's work is worth splitting into in total.
  //
  // Sizing shards by actor count alone produced 50 work units for 150 actors,
  // three actors each: far below what a scheduler round trip costs, and the
  // measured result was that more worker threads made the tick slower rather
  // than faster. This sizes them by estimated *work* instead, from the
  // per-actor cost the previous ticks actually took, so a quiet tick collapses
  // to a single unit and skips the barrier entirely.
  [[nodiscard]] size_t ComputeShardBudget() const;

  // Moves currentMinActorsToOffload toward wherever the pool is actually
  // paying for itself. Called once per tick, after the join, so it sees this
  // tick's measurements.
  void UpdateAdaptiveThreshold();

  // Whether this tick's movement should be taken on at all, decided once on
  // its first packet. False makes every SubmitMovement decline, which sends
  // ActionListener down the original inline path -- a genuinely different
  // thing from running the tick without the pool, which still pays for the
  // snapshot and the deferred join.
  [[nodiscard]] bool ShouldAcceptThisTick() const;

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

  // Rebuilt once per tick and read by every task. Const for the whole
  // parallel phase, like the snapshot itself.
  InterestManager::InterestPolicy policy;

  // Whether the most recent tick actually partitioned by area. On the inline
  // path the "clusters" are one synthetic bucket holding the whole server, so
  // its cost says nothing about any real area.
  bool lastTickOffloaded = false;

  ParallelMetrics metrics;
  uint64_t lastFailedTaskCount = 0;

  // Smoothed cost of processing one actor, in microseconds. Feeds the shard
  // budget. Zero until the first tick has been measured, which is treated as
  // "no estimate" rather than "free".
  double microsPerActorEma = 0.0;

  // Whether the pool has already been told this tick's batch is coming. The
  // hint is worth sending once, on the first submission, which is as early as
  // the fact is known.
  // Whether this tick's accept/decline decision has been taken yet, and what
  // it was. Taken once on the first submission and held for the whole tick, so
  // a tick never splits its relays between the deferred and the inline
  // ordering.
  bool tickDecisionMade = false;
  bool acceptingThisTick = true;

  // Actors accepted on the last tick that accepted any. Used to estimate this
  // tick's work before any of this tick's packets have arrived.
  size_t lastAcceptedActorCount = 0;

  // Submissions offered this tick and last tick, counted whether or not they
  // were taken on. This is what lets a shut gate notice a population growing
  // underneath it -- acceptances alone would freeze the moment it shuts.
  size_t attemptCountThisTick = 0;
  size_t lastAttemptCount = 0;

  // Ticks since the last one that accepted work. Drives the probe that stops
  // the gate latching shut on stale evidence.
  uint32_t ticksSinceAccept = 0;

  // Smoothed ratio of summed task time to the parallel phase's wall clock,
  // sampled only on ticks that actually used the pool. This is the gate's real
  // test: a ratio, so it does not drift with how fast the host is, and it
  // falls when the host is busy rather than rising the way absolute work does.
  double achievedSpeedupEma = 0.0;

  // How many tasks the previous tick pooled, used as the size hint for the
  // prime. Starts at 0 so the very first tick primes nothing and simply pays
  // the wakeup.
  size_t lastPooledUnitEstimate = 0;

  // Adaptive threshold for the number of actors required to offload to the pool.
  // Initially matches config.minActorsToOffload, but can be scaled dynamically
  // if adaptiveParallelism is enabled.
  size_t currentMinActorsToOffload = 0;

  // Consecutive offloaded ticks on which the pool did not pay for itself.
  // Reset by any profitable tick, so only a sustained run backs the threshold
  // off -- a single slow tick is noise, not evidence.
  uint32_t consecutiveUnprofitableTicks = 0;

  // Ticks left before the threshold may start decaying again. Set by a
  // backoff so the controller does not immediately walk back into the
  // population it just rejected.
  uint32_t backoffCooldownTicks = 0;
};

}
