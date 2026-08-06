#include "OffloadDispatcher.h"

#include "InterestManager.h"
#include <algorithm>
#include <chrono>
#include <iterator>
#include <spdlog/spdlog.h>

namespace MpParallel {

namespace {

[[nodiscard]] uint64_t NowMicros() noexcept
{
  return static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch())
      .count());
}

// How long an unvisited area keeps its cost estimate. At the server's ~1ms
// tick this is roughly a minute, long enough to survive a player stepping
// into a building and back out.
constexpr uint64_t kCostEntryMaxAgeTicks = 60000;

constexpr uint64_t kEvictIntervalTicks = 4096;

// Backstop against unbounded growth if the framework is enabled by an
// embedder that never calls ExecuteTick. At the real tick rate this is far
// above anything a legitimate tick produces, so hitting it means something is
// wrong rather than that the server is busy.
constexpr size_t kMaxPendingSubmissions = 200000;

// Weight of the newest sample in the per-actor cost estimate. Low enough that
// one tick which collided with a GC pause does not resize every shard, high
// enough to follow a crowd forming over a couple of seconds.
constexpr double kCostEmaAlpha = 0.2;

}

OffloadDispatcher::OffloadDispatcher(const ParallelConfig& config_)
  : config(config_)
{
  config.Normalize();
  currentMinActorsToOffload = config.minActorsToOffload;
  ResetPool();
}

OffloadDispatcher::~OffloadDispatcher() = default;

void OffloadDispatcher::ResetPool()
{
  // A disabled dispatcher spawns nothing. Turning it on at runtime is what
  // builds the threads.
  const size_t desired = config.enabled ? config.workerThreads : 0;

  if (pool && pool->GetWorkerCount() == desired &&
      pool->GetSpinMicros() == config.workerSpinMicros) {
    return;
  }

  // Destroy before constructing so the two pools never coexist and
  // double-subscribe the machine's cores.
  pool.reset();
  pool = std::make_unique<ThreadPool>(desired, config.workerSpinMicros);
  lastFailedTaskCount = 0;
}

void OffloadDispatcher::Reconfigure(const ParallelConfig& newConfig)
{
  DiscardPending();
  config = newConfig;
  config.Normalize();
  loadBalancer.Clear();
  // The cost model is calibrated against the old settings; keeping it would
  // size the first shards after a reconfigure from measurements of a
  // different configuration.
  microsPerActorEma = 0.0;
  currentMinActorsToOffload = config.minActorsToOffload;
  ResetPool();
}

void OffloadDispatcher::DiscardPending() noexcept
{
  snapshot.Clear();
  clusters.clear();
  workUnits.clear();
  tickDecisionMade = false;
  acceptingThisTick = true;
  for (ClusterOutput& output : unitOutputs) {
    output.Reset();
  }
}

bool OffloadDispatcher::SubmitMovement(const MovementSubmission& submission)
{
  if (!config.enabled) {
    return false;
  }

  // A relay with no packet behind it would produce sends of length zero, and
  // an actor with no owner has nothing to correct. Both mean the caller
  // built the submission wrong, so hand it back rather than guessing.
  if (submission.packetData == nullptr || submission.packetLength == 0) {
    return false;
  }

  if (snapshot.actors.size() >= kMaxPendingSubmissions) {
    spdlog::error("MpParallel: pending submissions hit the {} cap; is "
                  "ExecuteTick being called?",
                  kMaxPendingSubmissions);
    return false;
  }

  // Normally already decided, because the caller asked WillAcceptThisTick
  // before building the submission. Decided here too so a direct caller -- the
  // dispatcher's own tests -- still behaves.
  EnsureTickDecision();

  if (!acceptingThisTick) {
    return false;
  }

  ActorSnapshot actor;
  actor.formId = submission.formId;
  actor.idx = submission.idx;
  actor.ownerUserId = submission.ownerUserId;

  std::copy(std::begin(submission.currentPos), std::end(submission.currentPos),
            std::begin(actor.currentPos));
  std::copy(std::begin(submission.currentRot), std::end(submission.currentRot),
            std::begin(actor.currentRot));
  actor.currentWorldOrCell = submission.currentWorldOrCell;

  std::copy(std::begin(submission.proposedPos),
            std::end(submission.proposedPos), std::begin(actor.proposedPos));
  std::copy(std::begin(submission.proposedRot),
            std::end(submission.proposedRot), std::begin(actor.proposedRot));
  actor.proposedWorldOrCell = submission.proposedWorldOrCell;

  actor.teleportFlag = submission.teleportFlag;
  actor.isInJumpState = submission.isInJumpState;
  actor.isWeapDrawn = submission.isWeapDrawn;
  actor.isBlocking = submission.isBlocking;
  actor.isSneaking = submission.isSneaking;
  actor.isStanding = submission.isStanding;

  // Partition by where the server currently believes the actor is, not by
  // where the client is asking to go. A rejected update must not be able to
  // move an actor between clusters.
  actor.worldOrCell = submission.currentWorldOrCell;
  actor.area = AreaKey{ submission.currentWorldOrCell,
                        ToChunkCoord(submission.currentPos[0]),
                        ToChunkCoord(submission.currentPos[1]) };

  actor.packetOffset = static_cast<uint32_t>(snapshot.rawPacketBytes.size());
  actor.packetLength = static_cast<uint32_t>(submission.packetLength);
  snapshot.rawPacketBytes.insert(snapshot.rawPacketBytes.end(),
                                 submission.packetData,
                                 submission.packetData + submission.packetLength);

  actor.clusterIndex = kUnassignedCluster;
  snapshot.actors.push_back(actor);
  return true;
}

void OffloadDispatcher::SetPotentialTargets(std::vector<RelayTarget>&& targets)
{
  snapshot.relayTargets = std::move(targets);
  CommitPotentialTargets();
}

std::vector<RelayTarget>& OffloadDispatcher::BeginPotentialTargets() noexcept
{
  snapshot.relayTargets.clear();
  snapshot.relayChunks.clear();
  return snapshot.relayTargets;
}

void OffloadDispatcher::CommitPotentialTargets()
{
  std::sort(snapshot.relayTargets.begin(), snapshot.relayTargets.end(),
            [](const RelayTarget& a, const RelayTarget& b) {
              if (a.worldOrCell != b.worldOrCell) {
                return a.worldOrCell < b.worldOrCell;
              }
              if (a.chunkX != b.chunkX) {
                return a.chunkX < b.chunkX;
              }
              return a.chunkY < b.chunkY;
            });

  // One linear pass turns the sorted list into a per-chunk index. Workers
  // then search a table with one entry per occupied chunk instead of one per
  // connected player, which is the difference between a crowd of 400 in one
  // chunk costing log(400) per stencil probe and costing log(1).
  snapshot.relayChunks.clear();
  for (uint32_t i = 0; i < static_cast<uint32_t>(snapshot.relayTargets.size());
       ++i) {
    const RelayTarget& target = snapshot.relayTargets[i];
    const AreaKey key{ target.worldOrCell, target.chunkX, target.chunkY };
    if (!snapshot.relayChunks.empty() &&
        snapshot.relayChunks.back().key == key) {
      snapshot.relayChunks.back().end = i + 1;
      continue;
    }
    RelayChunk chunk;
    chunk.key = key;
    chunk.begin = i;
    chunk.end = i + 1;
    snapshot.relayChunks.push_back(chunk);
  }
}

void OffloadDispatcher::ExecuteTick(IOffloadSink& sink)
{
  ++snapshot.tickIndex;
  metrics.ResetTick();
  metrics.lastTickIndex = snapshot.tickIndex;
  ++metrics.totalTicks;
  tickDecisionMade = false;
  acceptingThisTick = true;

  // Rolled over before the early return below, because a tick that declined
  // everything leaves the snapshot empty and that is precisely the tick whose
  // attempt count the gate needs.
  lastAttemptCount = attemptCountThisTick;
  attemptCountThisTick = 0;
  metrics.lastAttemptCount = lastAttemptCount;

  if (snapshot.Empty()) {
    // A tick with nothing in it is not evidence the gate is right, so the
    // probe clock keeps running.
    if (lastAttemptCount > 0) {
      ++ticksSinceAccept;
    }
    ++metrics.totalDeclinedTicks;
    snapshot.Clear();
    return;
  }

  ticksSinceAccept = 0;
  metrics.lastActorCount = snapshot.actors.size();
  // Only ticks that accepted tell us anything about what a tick costs.
  lastAcceptedActorCount = snapshot.actors.size();

  RunUnits();
  JoinResults(sink);

  UpdateAdaptiveThreshold();
  metrics.lastAdaptiveThreshold = currentMinActorsToOffload;

  if (pool) {
    const uint64_t failed = pool->GetFailedTaskCount();
    if (failed > lastFailedTaskCount) {
      metrics.totalFailedTasks += failed - lastFailedTaskCount;
      lastFailedTaskCount = failed;
    }
  }

  if (snapshot.tickIndex % kEvictIntervalTicks == 0) {
    loadBalancer.EvictStale(snapshot.tickIndex, kCostEntryMaxAgeTicks);
  }

  if (config.metricsLogIntervalTicks > 0 &&
      snapshot.tickIndex % config.metricsLogIntervalTicks == 0) {
    spdlog::info(
      "MpParallel: tick={} actors={} clusters={} biggest={} units={} "
      "(pooled {}) relays={} throttled={} parallel={}us join={}us "
      "speedup={:.2f}x",
      metrics.lastTickIndex, metrics.lastActorCount, metrics.lastClusterCount,
      metrics.lastLargestClusterSize, metrics.lastWorkUnitCount,
      metrics.lastPooledUnitCount, metrics.lastRelayEdgesEmitted,
      metrics.lastRelayEdgesThrottled, metrics.lastParallelMicros,
      metrics.lastJoinMicros, metrics.GetLastSpeedup());
  }

  snapshot.Clear();
}

void OffloadDispatcher::EnsureTickDecision()
{
  if (tickDecisionMade) {
    return;
  }
  tickDecisionMade = true;
  acceptingThisTick = ShouldAcceptThisTick();

  // First packet of the tick: tell the workers a batch is coming. The rest of
  // ingest then runs while they wake, so by the time ExecuteTick publishes the
  // batch they are already spinning on it. That wakeup used to sit on the
  // critical path and was most of what made the offload lose below a few
  // hundred players.
  //
  // The size of the batch is not known yet -- the packets are still arriving
  // -- so the previous tick's is the estimate. Population moves slowly
  // relative to a tick, and being wrong only costs a spin or a wakeup.
  if (acceptingThisTick && pool) {
    pool->Prime(lastPooledUnitEstimate);
  }
}

bool OffloadDispatcher::WillAcceptThisTick()
{
  if (!config.enabled) {
    return false;
  }
  EnsureTickDecision();

  // Counted whether or not the tick is taken on: a declined tick still reveals
  // how many players are trying to move, which is what lets the gate notice a
  // growing population without having to accept work to find out.
  ++attemptCountThisTick;

  return acceptingThisTick;
}

bool OffloadDispatcher::ShouldAcceptThisTick() const
{
  // Declining is not the same as "run the tick without the pool", and the
  // difference is the whole point of this function.
  //
  // Skipping only the pool still flattens every packet into the snapshot and
  // still defers relays to the join, so the server pays for the split without
  // getting the parallelism. Measured, that is *worse* than never enabling the
  // feature: 577us against an inline 482us at 250 players. Declining instead
  // makes ActionListener take the original relay-then-validate path verbatim,
  // which is the real floor.
  //
  // On a scattered population -- which is what a live server looks like most
  // of the time, as opposed to the packed crowd the offload exists for -- this
  // is worth about 10% of the tick at 300 players, and no amount of tuning the
  // pool could have recovered it.
  if (!config.enabled) {
    return false;
  }

  // Declining also gives up interest management, which only exists on the
  // offloaded path. That sounds like it should make this decision a trade-off,
  // and it turns out not to be much of one: the work estimate is low exactly
  // when the population is scattered, and a scattered population is precisely
  // where interest management has least to do -- it sheds relays to *distant*
  // recipients, and a player alone in the woods has no distant recipients to
  // shed. The two line up rather than fight.
  //
  // An operator who wants interest management unconditionally can set
  // minOffloadWorkMicros to 0, which never declines.

  // No measurement yet: accept. The asymmetry is well established -- engaging
  // when we should not costs a few percent, failing to engage on a real crowd
  // costs more than half the tick.
  if (microsPerActorEma <= 0.0) {
    return true;
  }

  // The gate has been shut long enough that its evidence is stale. Take one
  // tick on to refresh it. Everything the decision rests on is measured only
  // on accepted ticks, so without this the gate cannot notice the world
  // changing under it.
  if (config.adaptiveProbeIntervalTicks > 0 &&
      ticksSinceAccept >= config.adaptiveProbeIntervalTicks) {
    return true;
  }

  // Attempts, not acceptances. A declined tick still tells us how many players
  // tried to move, and that is the half of the estimate which tracks a raid
  // forming. The other half -- what an actor costs, which rises as a crowd
  // packs together -- only a probe can refresh.
  const size_t actors =
    lastAttemptCount > 0 ? lastAttemptCount : lastAcceptedActorCount;
  if (actors == 0) {
    return true;
  }

  // A materially bigger population than the one the last verdict was formed
  // on. Do not wait out the probe interval for that -- the routine probe is
  // sized for density creeping up, and this is the fast case it would miss.
  if (lastAcceptedActorCount > 0 &&
      actors >= lastAcceptedActorCount + lastAcceptedActorCount / 2) {
    return true;
  }

  const double estimatedWork =
    microsPerActorEma * static_cast<double>(actors);
  if (estimatedWork < static_cast<double>(config.minOffloadWorkMicros)) {
    return false;
  }

  // The test that actually decides. Unlike the microsecond floor above, this
  // is a ratio of two measurements taken on the same machine under the same
  // conditions, so it neither drifts with how fast the host is nor inverts
  // when the host is busy.
  if (config.minOffloadSpeedup > 0.f && achievedSpeedupEma > 0.0) {
    return achievedSpeedupEma >=
      static_cast<double>(config.minOffloadSpeedup);
  }
  return true;
}

void OffloadDispatcher::UpdateAdaptiveThreshold()
{
  if (!config.adaptiveParallelism) {
    return;
  }

  if (!lastTickOffloaded) {
    // Nothing was measured this tick, so there is no new evidence. Walk the
    // threshold back down toward the floor so the pool gets probed again.
    //
    // Geometrically, not one actor per interval. Backing off sets the
    // threshold just above the current population, so a linear walk from 400
    // would take 370 intervals -- at the default of 10 ticks each, more than
    // a minute of running degraded. And degraded is not merely "no pool": the
    // snapshot is still built and the relays still deferred to the join, which
    // measures *worse* than never enabling the feature (577us against an
    // inline 482us at 250 players). Recovery has to be fast because the state
    // it recovers from is the expensive one.
    // Hold still for a while after a backoff. Without this the threshold
    // decays straight back through the population it just failed at, spends
    // adaptiveBackoffTicks gathering the same verdict, and backs off again --
    // measured at 50 players, that was 15 round trips in 150 ticks. The
    // cooldown makes a retry an occasional probe rather than a cycle.
    if (backoffCooldownTicks > 0) {
      --backoffCooldownTicks;
      return;
    }
    if (snapshot.tickIndex % config.adaptiveDecayTicks != 0) {
      return;
    }
    if (currentMinActorsToOffload > config.adaptiveThresholdFloor) {
      const size_t step = std::max<size_t>(currentMinActorsToOffload / 8, 1);
      currentMinActorsToOffload =
        currentMinActorsToOffload > config.adaptiveThresholdFloor + step
        ? currentMinActorsToOffload - step
        : config.adaptiveThresholdFloor;
    }
    return;
  }

  // Did the pool actually beat doing the same work on this thread?
  //
  // The comparison is the parallel phase's wall clock against the sum of its
  // tasks, and nothing else. The join is deliberately excluded: it runs
  // identically whether or not the work was spread across cores, so counting
  // it as a cost of parallelism is measuring a constant and blaming the pool
  // for it. Including it is what made this controller switch the offload off
  // at 100 players, where the offload was in fact 13% ahead.
  const uint64_t wall = metrics.lastParallelMicros;
  const auto work = static_cast<double>(metrics.lastAggregateTaskMicros);

  // A tick too small to time is not evidence either way.
  if (wall == 0 || work <= 0.0) {
    return;
  }

  if (static_cast<double>(wall) * config.adaptiveBias <= work) {
    consecutiveUnprofitableTicks = 0;
    return;
  }

  // One bad sample is noise -- a GC pause, the OS scheduling something else,
  // a tick that collided with a save. Requiring a run of them is what keeps a
  // hiccup from parking the server in the degraded path.
  if (++consecutiveUnprofitableTicks < config.adaptiveBackoffTicks) {
    return;
  }

  consecutiveUnprofitableTicks = 0;
  currentMinActorsToOffload = snapshot.actors.size() + 1;
  backoffCooldownTicks = config.adaptiveCooldownTicks;
  ++metrics.totalAdaptiveBackoffs;
}

size_t OffloadDispatcher::ComputeShardBudget() const
{
  // Enough pieces for the dynamic scheduler to balance, but not so many that
  // task overhead eats the gain. Two per slot is the usual ceiling: it
  // absorbs the variance between a shard of stationary players and a shard
  // in the middle of a fight.
  const size_t slots = pool ? pool->GetSlotCount() : 1;
  const size_t ceiling = config.maxShardsPerCluster > 0
    ? config.maxShardsPerCluster
    : slots * 2;

  // No measurement yet. Splitting across the slots is the reasonable guess,
  // and the estimate arrives after one tick.
  if (microsPerActorEma <= 0.0) {
    return std::max<size_t>(1, std::min(ceiling, slots));
  }

  const double estimatedMicros =
    microsPerActorEma * static_cast<double>(snapshot.actors.size());
  const double minShard = static_cast<double>(std::max<uint32_t>(
    config.minShardMicros, 1));

  const auto byWork = static_cast<size_t>(estimatedMicros / minShard);
  return std::max<size_t>(1, std::min(ceiling, byWork));
}

size_t OffloadDispatcher::ComputeShardCount(size_t clusterSize,
                                            size_t shardBudget) const
{
  if (clusterSize <= config.minShardActors || shardBudget <= 1) {
    return 1;
  }

  // The budget is for the whole tick, so each cluster gets the share of it
  // that its share of the population justifies. Without this a tick with
  // twenty small clusters would produce twenty times the intended number of
  // tasks.
  const size_t totalActors = std::max<size_t>(snapshot.actors.size(), 1);
  const size_t byShare = (shardBudget * clusterSize + totalActors - 1) /
    totalActors;

  const size_t byMinSize = clusterSize / config.minShardActors;
  return std::max<size_t>(1, std::min(byShare, byMinSize));
}

void OffloadDispatcher::BuildWorkUnits(bool allowSharding)
{
  workUnits.clear();

  const size_t shardBudget = allowSharding ? ComputeShardBudget() : 1;

  // Emitting units in cluster-index order, and in ascending member order
  // within a cluster, is what makes the join reproducible: the unit list is
  // a partition of the same sequence a single thread would have walked.
  for (uint32_t clusterIndex = 0;
       clusterIndex < static_cast<uint32_t>(clusters.size()); ++clusterIndex) {
    const AreaCluster& cluster = clusters[clusterIndex];
    const size_t size = cluster.Size();
    if (size == 0) {
      continue;
    }

    const size_t shardCount =
      allowSharding ? ComputeShardCount(size, shardBudget) : 1;
    const size_t perShard = (size + shardCount - 1) / shardCount;

    for (size_t begin = 0; begin < size; begin += perShard) {
      WorkUnit unit;
      unit.clusterIndex = clusterIndex;
      unit.begin = static_cast<uint32_t>(begin);
      unit.count = static_cast<uint32_t>(std::min(perShard, size - begin));
      workUnits.push_back(unit);
    }
  }

  if (unitOutputs.size() < workUnits.size()) {
    unitOutputs.resize(workUnits.size());
  }
  for (size_t i = 0; i < workUnits.size(); ++i) {
    unitOutputs[i].Reset();
  }
}

void OffloadDispatcher::RunUnits()
{
  const uint64_t parallelStart = NowMicros();

  // Two gates, and the second is the one that usually decides.
  //
  // Head count is only a floor. What settles whether the pool pays is how much
  // parallel work there is, which depends on how densely the population is
  // packed rather than on how large it is -- the relay term is quadratic in
  // how many players can see each other. The work estimate is the same one the
  // shard budget uses, so a scattered population produces a small number here
  // and a crowd a large one, with no extra measurement.
  //
  // Before the first measurement the estimate is unavailable, and the
  // asymmetry says which way to guess: engaging when we should not costs a few
  // percent, while failing to engage on a real crowd costs more than half the
  // tick. So an unwarmed estimate offloads.
  const bool enoughActors =
    snapshot.actors.size() >= currentMinActorsToOffload;
  const bool enoughWork = microsPerActorEma <= 0.0 ||
    microsPerActorEma * static_cast<double>(snapshot.actors.size()) >=
      static_cast<double>(config.minOffloadWorkMicros);

  const bool offload =
    pool && pool->GetWorkerCount() > 0 && enoughActors && enoughWork;
  lastTickOffloaded = offload;

  if (offload) {
    partitioner.Partition(snapshot.actors, config.clusterSeparationChunks,
                          clusters);
    metrics.lastChunkCount = partitioner.GetLastChunkCount();
    ++metrics.totalOffloadedTicks;
  } else {
    // One cluster covering everything. Below the offload threshold the
    // barrier costs more than the work, and partitioning would be wasted
    // effort on top of that.
    clusters.clear();
    clusters.emplace_back();
    AreaCluster& single = clusters.back();
    single.representative = snapshot.actors.front().area;
    single.chunkCount = 1;
    single.actorIndices.reserve(snapshot.actors.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(snapshot.actors.size());
         ++i) {
      snapshot.actors[i].clusterIndex = 0;
      single.actorIndices.push_back(i);
    }
    metrics.lastChunkCount = 1;
    ++metrics.totalInlineTicks;
    // Nothing will be pooled, so the next tick must not wake workers for a
    // batch that is not coming. Priming on 0 is a no-op.
    lastPooledUnitEstimate = 0;
  }

  metrics.lastClusterCount = clusters.size();
  for (const AreaCluster& cluster : clusters) {
    metrics.lastLargestClusterSize =
      std::max(metrics.lastLargestClusterSize, cluster.Size());
  }

  // Pressure is read from the previous tick's measurements, so it is a plain
  // input to the tasks and creates no dependency between them.
  const uint64_t perClusterBudget = std::max<uint64_t>(
    config.targetTickBudgetMicros / std::max<size_t>(clusters.size(), 1), 1);

  pressureByCluster.assign(clusters.size(), 0);
  if (config.adaptiveThrottling) {
    for (size_t i = 0; i < clusters.size(); ++i) {
      pressureByCluster[i] = loadBalancer.GetPressureLevel(
        clusters[i].representative, perClusterBudget,
        std::max<uint32_t>(config.maxThrottleSkipTicks, 1));
    }
  }

  BuildWorkUnits(offload);
  metrics.lastWorkUnitCount = workUnits.size();

  // Built once per tick rather than per relay edge. Squaring the configured
  // radii and reducing the tick index modulo every possible skip factor are
  // loop invariants over the N^2 inner loop, and the modulo is an integer
  // division that was being paid twice for every throttled edge.
  policy = InterestManager::InterestPolicy::Build(config, snapshot.tickIndex);

  if (!offload) {
    for (size_t unitIndex = 0; unitIndex < workUnits.size(); ++unitIndex) {
      const WorkUnit& unit = workUnits[unitIndex];
      const uint64_t taskStart = NowMicros();
      InterestManager::ProcessRange(
        snapshot, clusters[unit.clusterIndex].actorIndices.data() + unit.begin,
        unit.count, pressureByCluster[unit.clusterIndex], policy,
        unitOutputs[unitIndex]);
      unitOutputs[unitIndex].elapsedMicros = NowMicros() - taskStart;
    }
    // lastAggregateTaskMicros is accumulated by JoinResults, which walks the
    // same outputs; adding it here too would double count.
    metrics.lastParallelMicros = NowMicros() - parallelStart;
    return;
  }

  // Longest cluster first so the biggest pieces enter the queue before the
  // scraps. The pool hands tasks out dynamically from there.
  loadBalancer.BuildSchedule(clusters, schedule);

  clusterRank.assign(clusters.size(), 0);
  for (uint32_t rank = 0; rank < static_cast<uint32_t>(schedule.size());
       ++rank) {
    clusterRank[schedule[rank]] = rank;
  }

  tasks.clear();
  inlineUnitIndices.clear();
  tasks.reserve(workUnits.size());

  orderedUnitIndices.clear();
  orderedUnitIndices.reserve(workUnits.size());
  for (uint32_t unitIndex = 0;
       unitIndex < static_cast<uint32_t>(workUnits.size()); ++unitIndex) {
    orderedUnitIndices.push_back(unitIndex);
  }
  std::stable_sort(orderedUnitIndices.begin(), orderedUnitIndices.end(),
                   [this](uint32_t lhs, uint32_t rhs) {
                     return clusterRank[workUnits[lhs].clusterIndex] <
                       clusterRank[workUnits[rhs].clusterIndex];
                   });

  const size_t maxPooled = config.maxWorkUnitsPerTick > 0
    ? config.maxWorkUnitsPerTick
    : orderedUnitIndices.size();

  for (const uint32_t unitIndex : orderedUnitIndices) {
    const WorkUnit& unit = workUnits[unitIndex];
    // A unit that is the whole of a tiny cluster is cheaper to sweep up on
    // the calling thread than to hand through the scheduler.
    const bool tooSmall =
      clusters[unit.clusterIndex].Size() < config.minClusterActors;
    if (tooSmall || tasks.size() >= maxPooled) {
      inlineUnitIndices.push_back(unitIndex);
      continue;
    }

    tasks.emplace_back([this, unitIndex](size_t) {
      const WorkUnit& u = workUnits[unitIndex];
      const uint64_t taskStart = NowMicros();
      InterestManager::ProcessRange(
        snapshot, clusters[u.clusterIndex].actorIndices.data() + u.begin,
        u.count, pressureByCluster[u.clusterIndex], policy,
        unitOutputs[unitIndex]);
      unitOutputs[unitIndex].elapsedMicros = NowMicros() - taskStart;
    });
  }

  metrics.lastPooledUnitCount = tasks.size();
  lastPooledUnitEstimate = tasks.size();

  pool->Run(tasks);

  // The calling thread has finished its share of the pooled work by now, so
  // sweeping up the leftovers here costs nothing extra.
  for (const uint32_t unitIndex : inlineUnitIndices) {
    const WorkUnit& unit = workUnits[unitIndex];
    const uint64_t taskStart = NowMicros();
    InterestManager::ProcessRange(
      snapshot, clusters[unit.clusterIndex].actorIndices.data() + unit.begin,
      unit.count, pressureByCluster[unit.clusterIndex], policy,
      unitOutputs[unitIndex]);
    unitOutputs[unitIndex].elapsedMicros = NowMicros() - taskStart;
  }

  metrics.lastParallelMicros = NowMicros() - parallelStart;
}

void OffloadDispatcher::JoinResults(IOffloadSink& sink)
{
  const uint64_t joinStart = NowMicros();

  clusterMicros.assign(clusters.size(), 0);

  // Work-unit order: cluster index, then ascending member order within the
  // cluster. Both are fixed before any task starts, so the same inputs
  // produce the same sequence of world writes no matter how many cores ran
  // the tick or how the shards were distributed.
  for (size_t unitIndex = 0; unitIndex < workUnits.size(); ++unitIndex) {
    const WorkUnit& unit = workUnits[unitIndex];
    ClusterOutput& output = unitOutputs[unitIndex];

    if (!output.sends.empty()) {
      sink.SendRelayBatch(output.sends.data(), output.sends.size(),
                          snapshot.rawPacketBytes.data(),
                          snapshot.rawPacketBytes.size());
    }

    for (const MovementVerdict& verdict : output.verdicts) {
      if (verdict.actorIndex >= snapshot.actors.size()) {
        continue;
      }
      const ActorSnapshot& actor = snapshot.actors[verdict.actorIndex];

      if (verdict.accepted) {
        sink.ApplyMovement(actor);
      } else {
        ++metrics.lastRejectedMovements;
        if (verdict.needsCorrection) {
          sink.SendCorrection(actor);
        }
      }
    }

    metrics.lastRelayEdgesEmitted += output.emittedEdges;
    metrics.lastRelayEdgesThrottled += output.throttledEdges;
    metrics.lastAggregateTaskMicros += output.elapsedMicros;

    if (unit.clusterIndex < clusterMicros.size()) {
      clusterMicros[unit.clusterIndex] += output.elapsedMicros;
    }
  }

  // Cost is tracked per area, so a cluster's shards are summed back together
  // before being fed to the estimator. Otherwise splitting a busy area would
  // make it look cheap and it would never be recognised as under pressure.
  //
  // Only on an offloaded tick, though. The inline path fabricates a single
  // cluster covering the whole server and labels it with whichever area
  // happened to submit first; feeding that in would charge one arbitrary
  // chunk for every actor on the map, and the next offloaded tick would open
  // with that chunk apparently far over budget and start throttling players
  // who are doing nothing wrong.
  if (lastTickOffloaded) {
    for (size_t clusterIndex = 0; clusterIndex < clusters.size();
         ++clusterIndex) {
      loadBalancer.Observe(clusters[clusterIndex].representative,
                           clusterMicros[clusterIndex], snapshot.tickIndex);
    }
  }

  // Achieved speedup, and only from ticks that actually used the pool. On a
  // tick that ran everything on the calling thread the ratio is 1 by
  // construction, and feeding that in would drag the estimate below the gate's
  // threshold and keep it there -- the gate would conclude the pool does not
  // work from evidence gathered without it.
  if (lastTickOffloaded && metrics.lastParallelMicros > 0 &&
      metrics.lastAggregateTaskMicros > 0) {
    const double sample =
      static_cast<double>(metrics.lastAggregateTaskMicros) /
      static_cast<double>(metrics.lastParallelMicros);
    achievedSpeedupEma = achievedSpeedupEma <= 0.0
      ? sample
      : kCostEmaAlpha * sample + (1.0 - kCostEmaAlpha) * achievedSpeedupEma;
    metrics.lastAchievedSpeedup = achievedSpeedupEma;
  }

  // Per-actor cost drives next tick's shard budget. Measured rather than
  // assumed because it varies by two orders of magnitude between a lone
  // traveller and a player in a crowded market.
  if (!snapshot.actors.empty()) {
    const double sample = static_cast<double>(metrics.lastAggregateTaskMicros) /
      static_cast<double>(snapshot.actors.size());
    microsPerActorEma = microsPerActorEma <= 0.0
      ? sample
      : kCostEmaAlpha * sample + (1.0 - kCostEmaAlpha) * microsPerActorEma;
  }

  metrics.totalRelayEdgesEmitted += metrics.lastRelayEdgesEmitted;
  metrics.totalRelayEdgesThrottled += metrics.lastRelayEdgesThrottled;
  metrics.lastJoinMicros = NowMicros() - joinStart;
}

}
