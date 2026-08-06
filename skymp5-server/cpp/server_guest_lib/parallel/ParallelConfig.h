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

  // Decide whether to take movement on by *measuring both paths*, rather than
  // by comparing a statistic against a threshold.
  //
  // Every threshold tried here was wrong somewhere, and the last one was
  // provably wrong: packed 100 players wins at an achieved speedup of 1.66
  // while a scattered 300 loses at 2.20, so no cut-off takes the first without
  // taking the second. They are not separable by work per actor either -- both
  // measure 0.53us. The quantity that decides is the difference between what a
  // tick costs on each path, and nothing short of running both measures it.
  //
  // So periodically the dispatcher runs a trial: short alternating blocks of
  // accepted and declined ticks, timed end to end and normalised per mover.
  // Alternating is what makes it a fair comparison -- the population, the
  // spread and the machine's mood are all held constant across the pair in a
  // way that measuring one path today and the other tomorrow cannot manage.
  // Then it coasts on the verdict until the next trial.
  bool adaptiveParallelism = true;

  // How often to re-run the trial, in ticks. 1800 is thirty seconds at 60Hz.
  //
  // The verdict only goes stale when the *shape* of the population changes --
  // a crowd forming, a city emptying -- which is slow. Between trials the
  // dispatcher coasts, so the measurement costs nothing.
  uint32_t abTrialIntervalTicks = 1800;

  // Ticks per block, and blocks per trial. A trial is
  // abTrialBlockTicks * abTrialBlocks ticks long, half on each path.
  //
  // Blocks rather than strict tick-by-tick alternation because the accepted
  // path has warm-up inside it -- the shard budget is an EMA and the workers
  // have to be primed -- so a single accepted tick between declined ones would
  // measure that warm-up rather than the steady state. Four is enough to get
  // past it and short enough that the population cannot move much.
  //
  // At the defaults a trial is 48 ticks out of 1800, half of them on whichever
  // path turns out to be worse, so the whole mechanism costs well under half a
  // percent even when the paths differ by 15%.
  uint32_t abTrialBlockTicks = 4;
  uint32_t abTrialBlocks = 12;

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
  //
  // NOTE this is a floor, not the main gate. Head count turns out to be the
  // wrong quantity -- see minOffloadWorkMicros immediately below.
  size_t minActorsToOffload = 100;

  // Least estimated parallel work, in microseconds, that justifies engaging
  // the pool. This is the gate that actually decides.
  //
  // Head count does not decide whether the offload pays; *density* does. The
  // relay term is quadratic in how many players can see each other, not in how
  // many are logged in, so the same 300 players cost wildly different amounts
  // depending on whether they are stood in one market square or spread over a
  // province. Measured by unit/ParallelSimulation.cpp, 300 players:
  //
  //     packed into one chunk    ~90000 relays/tick   offload wins 1.9x
  //     spread realistically     ~15000 relays/tick   offload loses by 9%
  //
  // Gating on actors alone therefore cannot be right for both, and the
  // previous default of 100 -- calibrated entirely on the packed case --
  // engaged the pool on spread populations where it cost 74% at 100 players.
  //
  // The estimate is the same one the shard budget already uses: measured
  // per-actor cost from recent ticks times this tick's actor count. It falls
  // out low for a scattered population and high for a crowd, which is exactly
  // the distinction that matters.
  //
  // Swept against both workloads: `Where the work gate should sit` in
  // ParallelSimulation.cpp and `Work gate against a packed crowd` in
  // ParallelBenchmark.cpp. 100 is the value that is right for both; a figure
  // fitted to either alone is wrong for the other.
  //
  // 0 never declines, for an operator who wants the offloaded path
  // unconditionally -- typically for interest management, which only exists
  // there.
  //
  // This is a floor, not the main gate. It is in absolute microseconds, which
  // makes it machine-dependent in the wrong direction -- see
  // minOffloadSpeedup, which is the dimensionless test that actually decides.
  uint64_t minOffloadWorkMicros = 100;

  // Least parallel speedup the pool must be achieving for the offloaded path
  // to be worth taking.
  //
  // This replaces absolute work as the real gate, because absolute work leans
  // the wrong way. Both the work and the overhead it has to repay scale with
  // how fast the machine is, so a microsecond threshold calibrated on one host
  // is wrong on a slower one -- and worse, on a *contended* host the same
  // population looks like MORE work and opens the gate wider, exactly when
  // there are fewest spare cores to give it. Measured on a workstation that
  // picked up a game mid-session, 300 spread players went from 1.01x against
  // inline to 1.20x, with the gate happily accepting throughout.
  //
  // Achieved speedup is a ratio of two measurements taken on the same machine
  // in the same conditions, so it is immune to both. It also collapses three
  // separate questions into one: too little work leaves the barrier dominant
  // and the ratio near 1; plenty of work with free cores gives a high ratio;
  // plenty of work with contended cores gives a low one. Decline on a low
  // ratio is the right answer in all three.
  //
  // 2.5, and it is a compromise rather than a clean separation. Quiet machine:
  //
  //     packed  players    25     50    100    150    400
  //             speedup  0.75   0.93   1.66   3.51   7.79
  //             offload  loses  loses  wins   wins   wins
  //
  //     spread  players   100    200    300    500
  //             speedup     -      -   2.20   2.69
  //             offload     -      -  loses   wins
  //
  // No threshold gets every row right. Packed 100 wins at 1.66 while spread
  // 300 loses at 2.20, so any value low enough to take the first must take the
  // second. They are not distinguishable by work per actor either -- both
  // measure 0.53us. What differs is the overhead: a packed crowd is one
  // cluster and one work unit, a scattered population is thirty, and the
  // per-mover overhead measures 0.18us against 0.43us.
  //
  // 2.5 therefore buys the spread cases at the cost of the packed ones just
  // above break-even: spread 300 stops losing 13%, packed 100 stops winning
  // 3.5%. Scattered is what a live server looks like most of the time, and the
  // loss avoided is the larger number, so the trade is taken deliberately.
  //
  // Doing better than a compromise needs the decision to stop being a
  // threshold at all: measure what a tick costs on each path and compare them
  // directly, alternating so the population is held constant across the
  // comparison. That needs a tick-cost measurement the dispatcher does not
  // currently get, which is the next piece of work rather than a tuning change.
  //
  // 0 disables the test.
  float minOffloadSpeedup = 2.5f;

  // How long the gate may stay shut before it accepts one tick to find out
  // whether the world has changed under it.
  //
  // Without this the gate latches. A declined tick submits nothing, so nothing
  // measures what a tick would have cost, so the estimate that drives the
  // decision freezes at whatever the last accepted tick saw -- and a raid
  // forming underneath a shut gate could never reopen it. Measured on the
  // 60->500->60 raid cycle before this existed: 335us mean against 315us for
  // never declining at all, so the gate was costing more than it saved
  // precisely when the offload was worth most.
  //
  // 240 ticks, four seconds at 60Hz. Measured cost of the probe itself, mean
  // us/tick on a spread population, by `What the staleness probe costs`:
  //
  //     players   inline   no probe    240     60     20
  //     100         87.1       84.9   85.5   88.1   87.9
  //     200        220.8      217.5  215.4  224.0  236.9
  //
  // A probe every 60 ticks costs 3-4%; every 240 is inside the noise. The
  // reason a long interval is safe is that the probe is not what notices a
  // raid: the estimate has two terms, and the *attempt count* half is measured
  // on declined ticks too, so more players arriving reopens the gate on the
  // very next tick with no probe involved. The probe only refreshes the other
  // half -- what one actor costs, which rises as a crowd packs together -- and
  // that moves at walking pace. Four seconds of lag on it is not observable.
  //
  // 0 disables probing, which lets the gate latch on stale evidence. Only
  // sensible for a server whose density never changes.
  uint32_t adaptiveProbeIntervalTicks = 240;

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
