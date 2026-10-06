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

}

OffloadDispatcher::OffloadDispatcher(const ParallelConfig& config_)
  : config(config_)
{
  config.Normalize();
  ResetPool();
}

OffloadDispatcher::~OffloadDispatcher() = default;

void OffloadDispatcher::ResetPool()
{
  // A disabled dispatcher spawns nothing. Turning it on at runtime is what
  // builds the threads.
  const size_t desired = config.enabled ? config.workerThreads : 0;

  if (pool && pool->GetWorkerCount() == desired) {
    return;
  }

  // Destroy before constructing so the two pools never coexist and
  // double-subscribe the machine's cores.
  pool.reset();
  pool = std::make_unique<ThreadPool>(desired);
  lastFailedTaskCount = 0;
}

void OffloadDispatcher::Reconfigure(const ParallelConfig& newConfig)
{
  DiscardPending();
  config = newConfig;
  config.Normalize();
  loadBalancer.Clear();
  ResetPool();
}

void OffloadDispatcher::DiscardPending() noexcept
{
  snapshot.Clear();
  clusters.clear();
  workUnits.clear();
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
}

void OffloadDispatcher::ExecuteTick(IOffloadSink& sink)
{
  ++snapshot.tickIndex;
  metrics.ResetTick();
  metrics.lastTickIndex = snapshot.tickIndex;
  ++metrics.totalTicks;

  if (snapshot.Empty()) {
    snapshot.Clear();
    return;
  }

  metrics.lastActorCount = snapshot.actors.size();

  RunUnits();
  JoinResults(sink);

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

size_t OffloadDispatcher::ComputeShardCount(size_t clusterSize) const
{
  if (clusterSize <= config.minShardActors) {
    return 1;
  }

  // Enough pieces for the dynamic scheduler to balance, but not so many that
  // task overhead eats the gain. Two per slot is the usual sweet spot: it
  // absorbs the variance between a shard of stationary players and a shard
  // in the middle of a fight.
  const size_t slots = pool ? pool->GetSlotCount() : 1;
  const size_t configured = config.maxShardsPerCluster > 0
    ? config.maxShardsPerCluster
    : slots * 2;

  const size_t byMinSize = clusterSize / config.minShardActors;
  return std::max<size_t>(1, std::min(configured, byMinSize));
}

void OffloadDispatcher::BuildWorkUnits(bool allowSharding)
{
  workUnits.clear();

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

    const size_t shardCount = allowSharding ? ComputeShardCount(size) : 1;
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

  const bool offload = pool && pool->GetWorkerCount() > 0 &&
    snapshot.actors.size() >= config.minActorsToOffload;

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

  if (!offload) {
    for (size_t unitIndex = 0; unitIndex < workUnits.size(); ++unitIndex) {
      const WorkUnit& unit = workUnits[unitIndex];
      const uint64_t taskStart = NowMicros();
      InterestManager::ProcessRange(
        snapshot, clusters[unit.clusterIndex].actorIndices.data() + unit.begin,
        unit.count, pressureByCluster[unit.clusterIndex], config,
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
        u.count, pressureByCluster[u.clusterIndex], config,
        unitOutputs[unitIndex]);
      unitOutputs[unitIndex].elapsedMicros = NowMicros() - taskStart;
    });
  }

  metrics.lastPooledUnitCount = tasks.size();

  pool->Run(tasks);

  // The calling thread has finished its share of the pooled work by now, so
  // sweeping up the leftovers here costs nothing extra.
  for (const uint32_t unitIndex : inlineUnitIndices) {
    const WorkUnit& unit = workUnits[unitIndex];
    const uint64_t taskStart = NowMicros();
    InterestManager::ProcessRange(
      snapshot, clusters[unit.clusterIndex].actorIndices.data() + unit.begin,
      unit.count, pressureByCluster[unit.clusterIndex], config,
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

    for (const OutboundSend& send : output.sends) {
      if (send.byteLength == 0 ||
          static_cast<size_t>(send.byteOffset) + send.byteLength >
            snapshot.rawPacketBytes.size()) {
        continue;
      }
      sink.SendRelay(send.userId,
                     snapshot.rawPacketBytes.data() + send.byteOffset,
                     send.byteLength, send.reliable);
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
  for (size_t clusterIndex = 0; clusterIndex < clusters.size();
       ++clusterIndex) {
    loadBalancer.Observe(clusters[clusterIndex].representative,
                         clusterMicros[clusterIndex], snapshot.tickIndex);
  }

  metrics.totalRelayEdgesEmitted += metrics.lastRelayEdgesEmitted;
  metrics.totalRelayEdgesThrottled += metrics.lastRelayEdgesThrottled;
  metrics.lastJoinMicros = NowMicros() - joinStart;
}

}
