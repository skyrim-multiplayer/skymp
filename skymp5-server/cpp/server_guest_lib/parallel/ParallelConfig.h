#pragma once
// For kMinSafeSeparationChunks, which Normalize clamps against.
#include "AreaKey.h"
// For kDefaultSpinMicros, the default of workerSpinMicros.
#include "ThreadPool.h"
#include <cstddef>
#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <string>

namespace MpParallel {

constexpr size_t kMaxWorkerThreads = 32;

// Parsed from the "parallelism" object of server-settings.json.
//
// The framework is opt-in. With `enabled` false every entry point becomes a
// no-op and the server behaves exactly as it did before, which keeps the
// default deployment risk at zero.
struct ParallelConfig
{
  bool enabled = false;

  // Dynamically tune minActorsToOffload during runtime based on actual 
  // execution metrics. Defaults to true as the penalty for configuring
  // minActorsToOffload too high is heavily asymmetric.
  bool adaptiveParallelism = true;

  // The overhead tolerance factor. E.g. 1.05 means we allow parallel execution
  // to be up to 5% slower than the sequential estimate before bailing out.
  //
  // The comparison is the parallel phase's wall clock against the sum of its
  // own tasks. The join is not part of it: the join runs identically whether
  // or not the work was spread across cores, so charging it to the pool would
  // be blaming parallelism for a constant.
  float adaptiveBias = 1.05f;

  // How many *consecutive* offloaded ticks must fail to pay for themselves
  // before the threshold backs off.
  //
  // One bad sample is noise -- a GC pause, the OS scheduling something else,
  // a tick that collided with a save. Acting on a single sample parked the
  // server in the degraded path for a minute at a time, and the degraded path
  // measures worse than never enabling the feature.
  uint32_t adaptiveBackoffTicks = 8;

  // How frequently (in ticks) we decay the threshold to probe offloading again.
  uint32_t adaptiveDecayTicks = 10;

  // Ticks to hold still after a backoff before the threshold starts decaying
  // again. Without it the threshold walks straight back through the
  // population it just rejected, re-gathers the same verdict, and backs off
  // again -- 15 round trips in 150 ticks when measured at 50 players.
  uint32_t adaptiveCooldownTicks = 300;

  // The minimum minActorsToOffload we will ever decay down to.
  size_t adaptiveThresholdFloor = 30;

  // 0 means "auto": an estimate of physical cores minus one for the Node/V8
  // main thread, capped at kMaxAutoWorkerThreads (8). See ParallelConfig.cpp
  // for why the cap is there and what it was measured against -- past it,
  // more threads made the tick markedly slower rather than faster. An
  // explicit value here is bounded only by kMaxWorkerThreads.
  size_t workerThreads = 0;

  // Below this many tracked actors the fork/join barrier costs more than the
  // work it distributes, so the dispatcher runs everything on the calling
  // thread.
  //
  // Measured, not guessed. unit/ParallelBenchmark.cpp times the inline path
  // against the offloaded one with every player in a single chunk:
  //
  //     players    25     50    100    150    250    400
  //     was      0.25x  0.43x  0.66x  0.81x  0.88x  1.10x
  //     now      0.85x  0.88x  0.99x  1.29x  1.87x  2.17x
  //
  // Break-even used to sit near 300-400 and now sits near 100. What moved it
  // was not the parallel phase, which was always small, but the three serial
  // costs around it: the barrier (a condvar round trip every tick, ~50us of a
  // 64us parallel phase at 150 players), the join (three virtual calls and a
  // throw-if-null send-target lookup per relay edge), and shards sized by
  // actor count rather than by work.
  //
  // Note this threshold only turns the *thread pool* off. Flattening packets
  // into the snapshot still happens, so below it the feature is a few percent
  // of tick time in exchange for interest management's bandwidth reduction.
  // A server that never approaches 100 concurrent movers and does not need
  // that reduction should leave `enabled` false.
  //
  // Re-run `./unit/unit "[ParallelBench]"` on the target hardware before
  // changing it.
  size_t minActorsToOffload = 100;

  // Clusters smaller than this are merged into the inline residual batch
  // rather than being scheduled as their own task.
  size_t minClusterActors = 4;

  // Fewest actors a shard of a large cluster may carry.
  //
  // A single crowded area is typically most of a tick's work, so a scheme
  // that could only parallelise across areas would be capped at whatever
  // fraction the quiet areas contribute. Splitting a busy cluster into
  // several ranges is what makes the offload scale with core count instead
  // of with the number of populated areas.
  size_t minShardActors = 4;

  // Upper bound on how many shards one cluster may be split into.
  // 0 means "auto": twice the slot count, which gives the dynamic scheduler
  // enough pieces to balance without paying for needless task overhead.
  size_t maxShardsPerCluster = 0;

  // Smallest amount of estimated work, in microseconds, that justifies making
  // a separate shard out of it.
  //
  // Shards used to be sized purely by actor count, which produced 50 work
  // units for 150 actors -- three actors and a couple of microseconds each,
  // well under what a scheduler round trip costs. The measured symptom was
  // that raising the worker count from 8 to 16 made the tick *slower*. Sizing
  // by estimated work instead means a quiet tick collapses to one unit and
  // skips the barrier entirely, and a busy one still splits far enough to
  // fill every core.
  //
  // 20 is measured, by the `Shard granularity` case in ParallelBenchmark.cpp:
  //
  //     players    inline   8us    12us   20us   30us   60us
  //     100         100     147    131    131    142    142
  //     150         198     317    253    230    232    278
  //
  // It is the optimum for the 100-250 band. Larger populations do slightly
  // better on larger shards -- at 400 the best value was nearer 60 -- but by
  // then the serial join dominates the tick and the shard size stops
  // mattering much either way.
  uint32_t minShardMicros = 20;

  // --- interest management -----------------------------------------------
  //
  // Distance-based update-rate reduction, applied every tick regardless of
  // load. This is the lever that actually moves the needle: relay volume is
  // the N^2 term, and emitting those sends is serial no matter how many cores
  // the decisions were spread over. Measured on 400 players in one chunk it
  // cut relays from 160k to 66k per tick and the tick from 1120us to 682us.
  //
  // Unlike adaptiveThrottling below, this does not wait for the server to be
  // in trouble, because a player 60 metres away does not need 60 position
  // updates a second even on an idle server.
  bool interestManagement = true;

  // Recipients closer than this always receive every update. Sized a little
  // over half a chunk so that anything a player is realistically fighting,
  // trading with, or watching stays at full fidelity.
  float interestFullRateUnits = 2048.f;

  // Hard ceiling on how far apart interest management may space an update.
  // At 4 a distant player still gets ~15 updates a second at a 60Hz tick.
  uint32_t maxInterestSkipTicks = 4;

  // Chebyshev distance, in 4096-unit chunks, that must separate two clusters.
  // Clamped up to kMinSafeSeparationChunks.
  int32_t clusterSeparationChunks = 4;

  // 0 means unlimited. Work units beyond the limit are processed inline on
  // the calling thread instead of going through the scheduler.
  size_t maxWorkUnitsPerTick = 0;

  // Degrade relay frequency for distant neighbours when a cluster exceeds its
  // share of the tick budget, instead of letting the whole server stall.
  bool adaptiveThrottling = true;

  // Per-tick wall-clock target for the parallel phase, in microseconds.
  uint64_t targetTickBudgetMicros = 8000;

  // Squared distance beyond which a relay becomes eligible for throttling.
  // Default is one exterior cell (4096 units).
  float throttleDistanceUnits = 4096.f;

  // Maximum number of ticks a throttled relay may be held back.
  uint32_t maxThrottleSkipTicks = 3;

  // Emit a per-tick summary line at this interval. 0 disables reporting.
  uint32_t metricsLogIntervalTicks = 0;

  // How long a worker stays hot after being told a batch is coming, before it
  // parks again. Sized to cover packet ingest, which is the gap it exists to
  // bridge; see ThreadPool.h. 0 disables spinning entirely.
  uint32_t workerSpinMicros = kDefaultSpinMicros;

  // Resolves workerThreads==0 to a concrete count and clamps every field to
  // its documented range. Idempotent.
  void Normalize();

  // Reads the "parallelism" object if present. Unknown keys are ignored,
  // missing keys keep their defaults, and a malformed value throws
  // std::runtime_error naming the offending key.
  static ParallelConfig FromServerSettings(const nlohmann::json& serverSettings);

  // Human-readable one-liner for the startup log.
  [[nodiscard]] std::string Describe() const;
};

}
