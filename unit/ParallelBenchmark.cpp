#include "TestUtils.hpp"
// PartOne.h only forward-declares MessageSerializer; we call Serialize on it.
#include "MessageSerializerFactory.h"
#include "UpdateMovementMessage.h"
#include "parallel/OffloadDispatcher.h"
#include "parallel/ParallelConfig.h"
#include "parallel/ParallelMetrics.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <slikenet/BitStream.h>
#include <vector>

// Measures what the parallel area offload actually buys, instead of arguing
// about it. Hidden behind the "[.]" tag so ctest never picks it up; run it
// explicitly:
//
//   ./unit/unit "[ParallelBench]"
//
// The scenario is the one the framework exists for: every player standing in
// the same chunk, every player sending a movement update every tick. That is
// the N^2 relay case -- N senders x N recipients.
//
// Both configurations are timed over the same unit of work: the full ingest
// of every player's packet PLUS PartOne::Tick. That matters for fairness,
// because the inline path relays during ingest while the offloaded path defers
// relays to the join inside Tick. Timing only one half would flatter whichever
// path was measured.

namespace {

class NullSendTarget : public Networking::ISendTarget
{
public:
  void Send(Networking::UserId, Networking::PacketData, size_t, bool) override
  {
    // Deliberately does nothing: we are measuring server-side relay cost, not
    // the network stack. Both configurations pay the same zero here.
    ++sendCount;
  }
  uint64_t sendCount = 0;
};

struct Sample
{
  double perTickMicros = 0.0;
  uint64_t relays = 0;
  size_t clusters = 0;
  size_t units = 0;
  size_t biggest = 0;
  double reportedSpeedup = 0.0;
  // Where the tick actually goes. ingest is inferred: whatever is left after
  // the dispatcher's own two phases.
  uint64_t parallelMicros = 0;
  uint64_t joinMicros = 0;
  uint64_t aggregateTaskMicros = 0;
};

// Feeds a movement update the way a real client does: a binary-serialized
// UpdateMovementMessage. TestUtils::DoMessage sends JSON instead, which is the
// legacy path -- a live client produced no JSON packets at all, and
// PacketParser warns the first time it sees one. Benchmarking JSON would
// measure a path production never takes.
void SendBinaryMovement(PartOne& partOne, Networking::UserId userId,
                        uint32_t idx, float x, float y)
{
  UpdateMovementMessage msg;
  msg.idx = idx;
  msg.data.worldOrCell = 0x3c;
  msg.data.pos = { x, y, 0.f };
  msg.data.rot = { 0.f, 0.f, 0.f };
  msg.data.direction = 0.f;
  msg.data.healthPercentage = 1.f;
  msg.data.speed = 0.f;
  msg.data.runMode = "Standing";
  msg.data.isInJumpState = false;
  msg.data.isSneaking = false;
  msg.data.isBlocking = false;
  msg.data.isWeapDrawn = false;
  msg.data.isDead = false;

  SLNet::BitStream stream;
  PartOne::GetMessageSerializerInstance().Serialize(msg, stream);

  PartOne::HandlePacket(
    &partOne, userId, Networking::PacketType::Message,
    reinterpret_cast<Networking::PacketData>(stream.GetData()),
    stream.GetNumberOfBytesUsed());
}

// Same update over the legacy JSON encoding, for an apples-to-apples
// comparison of ingest cost between the two wire formats.
void SendJsonMovement(PartOne& partOne, Networking::UserId userId, uint32_t idx,
                      float x, float y)
{
  auto m = jMovement;
  m["idx"] = idx;
  m["data"]["pos"] = { x, y, 0.f };
  DoMessage(partOne, userId, m);
}

MpParallel::ParallelConfig MakeConfig(size_t workers)
{
  MpParallel::ParallelConfig config;
  config.enabled = true;
  config.workerThreads = workers;
  config.minActorsToOffload = 1;
  config.minClusterActors = 1;
  config.minShardActors = 2;
  // Throttling would change the amount of work done, making the two
  // configurations incomparable. Keep the workload identical.
  config.adaptiveThrottling = false;
  // Likewise for the offload-threshold controller. It moves the threshold
  // during the run, so leaving it on means a row of this table is an average
  // over whatever settings it happened to pass through -- and the pinned
  // `minActorsToOffload = 1` above stops meaning anything. The controller has
  // its own case; here it must hold still so everything else is comparable.
  config.adaptiveParallelism = false;
  // And the work gate, for the same reason. These cases exist to measure what
  // the offloaded path costs at each population, including the populations
  // where it is a bad idea -- that is the whole point of the speedup column.
  // With the gate at its default the dispatcher would correctly decline the
  // small ones and the table would report the inline path against itself.
  // `Work gate against a packed crowd` sets it explicitly and is what measures
  // the gate.
  config.minOffloadWorkMicros = 0;
  config.minOffloadSpeedup = 0.f;
  config.Normalize();
  return config;
}

// Same, but with the adaptive threshold controller left on, for the cases that
// are specifically measuring it.
MpParallel::ParallelConfig MakeAdaptiveConfig(size_t workers)
{
  MpParallel::ParallelConfig config = MakeConfig(workers);
  config.adaptiveParallelism = true;
  config.Normalize();
  return config;
}

Sample RunScenario(int players, bool parallel, size_t workers, int ticks,
                   bool useJson = false, bool interestMgmt = false,
                   uint32_t minShardMicros = 0, size_t maxShards = 0)
{
  PartOne partOne;
  NullSendTarget sendTarget;
  partOne.SetSendTarget(&sendTarget);

  if (parallel) {
    MpParallel::ParallelConfig config = MakeConfig(workers);
    config.interestManagement = interestMgmt;
    if (minShardMicros > 0) {
      config.minShardMicros = minShardMicros;
    }
    if (maxShards > 0) {
      config.maxShardsPerCluster = maxShards;
    }
    config.Normalize();
    partOne.ConfigureParallelism(config);
  }

  // Everyone in one chunk of Tamriel, so the partitioner produces a single
  // cluster and sharding is what has to carry the parallelism.
  //
  // Spread across most of the chunk rather than heaped into a few hundred
  // units: a real crowd occupies a market square or a city district, and the
  // spread is what determines whether interest management can do anything.
  const int side = static_cast<int>(std::ceil(std::sqrt(
    static_cast<double>(players))));
  const float spacing = 3800.f / static_cast<float>(std::max(side - 1, 1));
  auto posX = [&](int i) { return static_cast<float>(i % side) * spacing; };
  auto posY = [&](int i) { return static_cast<float>(i / side) * spacing; };

  std::vector<uint32_t> idx(players);
  for (int i = 0; i < players; ++i) {
    DoConnect(partOne, static_cast<Networking::UserId>(i));
    const uint32_t formId = 0xff000000 + static_cast<uint32_t>(i);
    partOne.CreateActor(formId, { posX(i), posY(i), 0.f }, 0.f, 0x3c);
    partOne.SetUserActor(static_cast<Networking::UserId>(i), formId);
    idx[i] = dynamic_cast<MpActor*>(
               partOne.worldState.LookupFormById(formId).get())
               ->GetIdx();
  }

  auto oneTick = [&] {
    for (int i = 0; i < players; ++i) {
      // Nudge the position each tick so the update is not a no-op.
      const float x = posX(i) + 1.f;
      const float y = posY(i) + 1.f;
      const auto userId = static_cast<Networking::UserId>(i);
      if (useJson) {
        SendJsonMovement(partOne, userId, idx[i], x, y);
      } else {
        SendBinaryMovement(partOne, userId, idx[i], x, y);
      }
    }
    partOne.Tick();
  };

  // Warm up: first ticks pay for subscription setup and allocator growth.
  for (int w = 0; w < 5; ++w) {
    oneTick();
  }

  sendTarget.sendCount = 0;

  // The phase counters are per-tick and get overwritten, so they are summed
  // as we go. Reading a handful of uint64s per tick is nothing against a tick
  // measured in hundreds of microseconds, and the alternative -- reporting
  // the last tick's breakdown against an averaged tick time -- produced
  // impossible numbers whenever the final tick happened to be an outlier.
  uint64_t parallelSum = 0;
  uint64_t joinSum = 0;
  uint64_t taskSum = 0;

  const auto start = std::chrono::steady_clock::now();
  for (int t = 0; t < ticks; ++t) {
    oneTick();
    if (parallel) {
      const MpParallel::ParallelMetrics& m = partOne.GetParallelMetrics();
      parallelSum += m.lastParallelMicros;
      joinSum += m.lastJoinMicros;
      taskSum += m.lastAggregateTaskMicros;
    }
  }
  const auto elapsed = std::chrono::steady_clock::now() - start;

  Sample sample;
  sample.perTickMicros =
    std::chrono::duration<double, std::micro>(elapsed).count() / ticks;
  sample.relays = sendTarget.sendCount / static_cast<uint64_t>(ticks);

  if (parallel) {
    const MpParallel::ParallelMetrics& m = partOne.GetParallelMetrics();
    sample.clusters = m.lastClusterCount;
    sample.units = m.lastWorkUnitCount;
    sample.biggest = m.lastLargestClusterSize;
    sample.reportedSpeedup = m.GetLastSpeedup();

    const auto n = static_cast<uint64_t>(ticks);
    sample.parallelMicros = parallelSum / n;
    sample.joinMicros = joinSum / n;
    sample.aggregateTaskMicros = taskSum / n;
  }
  return sample;
}

}

TEST_CASE("Parallel offload throughput vs inline", "[.][ParallelBench]")
{
  // Extended past 150 to find the crossover: the offload only earns its
  // barrier overhead once the parallel phase is large enough to amortize it.
  const std::vector<int> populations = { 25, 50, 100, 150, 250, 400 };
  // At 40 ticks the run-to-run spread on this machine was about 12%, wider
  // than several of the effects being reported. 150 brings it under 3%.
  constexpr int kTicks = 150;

  const size_t hw = std::thread::hardware_concurrency();
  std::printf("\n  hardware_concurrency = %zu\n", hw);
  std::printf("  %d ticks per measurement, all players in one chunk\n\n",
              kTicks);
  std::printf("  %-8s %8s %12s %12s %9s  %s\n", "players", "relays",
              "inline us", "parallel us", "speedup", "clusters/units");
  std::printf("  %s\n", std::string(72, '-').c_str());

  for (int players : populations) {
    const Sample inlineRun = RunScenario(players, false, 0, kTicks);
    const Sample parallelRun = RunScenario(players, true, 0, kTicks);

    // Both configurations must have done the same amount of relaying, or the
    // comparison is meaningless.
    REQUIRE(parallelRun.relays == inlineRun.relays);

    const double speedup = parallelRun.perTickMicros > 0.0
      ? inlineRun.perTickMicros / parallelRun.perTickMicros
      : 0.0;

    std::printf("  %-8d %8llu %12.1f %12.1f %8.2fx  %zu/%zu (biggest %zu)\n",
                players, static_cast<unsigned long long>(inlineRun.relays),
                inlineRun.perTickMicros, parallelRun.perTickMicros, speedup,
                parallelRun.clusters, parallelRun.units, parallelRun.biggest);

    // Where did the parallel tick's time actually go? Anything not accounted
    // for by the dispatcher's phases is serial ingest -- flattening actors and
    // copying every relay edge into the snapshot.
    const double accounted = static_cast<double>(parallelRun.parallelMicros +
                                                 parallelRun.joinMicros);
    std::printf("           breakdown: ingest~%.1f  parallel=%llu  join=%llu"
                "  tasksum=%llu\n",
                parallelRun.perTickMicros - accounted,
                static_cast<unsigned long long>(parallelRun.parallelMicros),
                static_cast<unsigned long long>(parallelRun.joinMicros),
                static_cast<unsigned long long>(
                  parallelRun.aggregateTaskMicros));

    // The crowd must have formed one cluster, otherwise the benchmark is not
    // measuring what it claims to.
    //
    // Unit count is deliberately not asserted to be > 1. Shards are sized by
    // estimated work, so a small population collapsing to a single unit -- and
    // skipping the barrier entirely -- is the intended outcome, not a sign the
    // benchmark stopped exercising sharding. The larger populations below do
    // still shard, and the printed column shows it.
    REQUIRE(parallelRun.clusters == 1);
    REQUIRE(parallelRun.units >= 1);
  }
  std::printf("\n");
}

TEST_CASE("Interest management: what cutting relays actually buys",
          "[.][ParallelBench]")
{
  // At 400 players the join emits 160k relays serially and dominates the
  // tick. Parallelising decisions cannot help that; sending less can. This
  // measures the only lever that attacks the N^2 term directly.
  constexpr int kTicks = 150;
  std::printf("\n  relay volume vs tick cost, 400 players in one chunk\n\n");
  std::printf("  %-26s %10s %12s\n", "configuration", "relays", "us/tick");
  std::printf("  %s\n", std::string(52, '-').c_str());

  for (int players : { 150, 400 }) {
    const Sample inlineRun = RunScenario(players, false, 0, kTicks);
    const Sample plain = RunScenario(players, true, 0, kTicks, false, false);
    const Sample managed = RunScenario(players, true, 0, kTicks, false, true);

    std::printf("  %d players\n", players);
    std::printf("  %-28s %10llu %12.1f\n", "  inline (baseline)",
                static_cast<unsigned long long>(inlineRun.relays),
                inlineRun.perTickMicros);
    std::printf("  %-28s %10llu %12.1f\n", "  offload only",
                static_cast<unsigned long long>(plain.relays),
                plain.perTickMicros);
    std::printf("  %-28s %10llu %12.1f\n", "  offload + interest mgmt",
                static_cast<unsigned long long>(managed.relays),
                managed.perTickMicros);
    std::printf("  -> relays -%.0f%%, %.2fx faster than inline\n\n",
                100.0 *
                  (1.0 - static_cast<double>(managed.relays) /
                           static_cast<double>(std::max<uint64_t>(plain.relays, 1))),
                inlineRun.perTickMicros / managed.perTickMicros);

    // Rate limiting must reduce traffic, never silence it.
    REQUIRE(managed.relays > 0);
    REQUIRE(managed.relays <= plain.relays);
  }
}

TEST_CASE("Wire format ingest cost: binary vs legacy JSON",
          "[.][ParallelBench]")
{
  // MessageSerializer::Deserialize dispatches binary in O(1) on the type byte,
  // but for JSON it walks the deserializer table, and each candidate allocates
  // a std::string AND constructs a fresh simdjson::dom::parser before
  // re-parsing the whole message. UpdateMovement is type 2, so a movement
  // packet pays that twice before it matches.
  constexpr int kTicks = 40;
  std::printf("\n  ingest cost per player-packet, offload disabled\n\n");
  std::printf("  %-8s %14s %14s %10s\n", "players", "binary us/tick",
              "json us/tick", "json cost");
  std::printf("  %s\n", std::string(50, '-').c_str());

  for (int players : { 25, 50, 100, 150 }) {
    const Sample bin = RunScenario(players, false, 0, kTicks, false);
    const Sample json = RunScenario(players, false, 0, kTicks, true);
    REQUIRE(bin.relays == json.relays);
    std::printf("  %-8d %14.1f %14.1f %9.2fx\n", players, bin.perTickMicros,
                json.perTickMicros, json.perTickMicros / bin.perTickMicros);
  }
  std::printf("\n");
}

TEST_CASE("Shard granularity: how small a work unit still pays",
          "[.][ParallelBench]")
{
  // Shards are sized by estimated work rather than by actor count, and
  // minShardMicros is the knob that says how small a piece is still worth
  // handing to the scheduler. Too large and the tick never splits at all; too
  // small and the batch is more dispatch than work. This is where the default
  // comes from -- re-run it on the target hardware before changing it.
  //
  // More ticks than the other cases: the differences being resolved here are
  // tens of microseconds on a few hundred, and at 40 ticks the run-to-run
  // spread was wider than the effect.
  constexpr int kTicks = 150;
  std::printf("\n  us/tick by minShardMicros (0 = inline baseline)\n\n");
  std::printf("  %-8s %10s", "players", "inline");
  for (uint32_t m : { 8u, 12u, 20u, 30u, 60u }) {
    std::printf(" %7uus", m);
  }
  std::printf("\n  %s\n", std::string(64, '-').c_str());

  for (int players : { 50, 100, 150, 250, 400 }) {
    const Sample baseline = RunScenario(players, false, 0, kTicks);
    std::printf("  %-8d %9.1f", players, baseline.perTickMicros);
    for (uint32_t m : { 8u, 12u, 20u, 30u, 60u }) {
      const Sample run =
        RunScenario(players, true, 0, kTicks, false, false, m);
      REQUIRE(run.relays == baseline.relays);
      std::printf("  %5.1f/%-3zu", run.perTickMicros, run.units);
    }
    std::printf("\n");
  }
  std::printf("\n  (cell is us/tick and work-unit count)\n\n");
}

// One segment of a load profile: hold this many senders for this many ticks.
struct LoadSegment
{
  int ticks = 0;
  int activePlayers = 0;
};

// Drives a population that changes over time, which is what any setting tuned
// once cannot follow.
//
// Every player stays connected throughout -- so the recipient set, and
// therefore the per-sender work, is constant -- and only the number of them
// sending movement each tick varies. That is the quantity the shard budget is
// derived from, so it is what moves the optimum.
//
// Returns total wall clock for the whole profile, in microseconds. Totals
// rather than per-tick averages because segments differ in length and the
// question is which configuration finishes the whole shift faster.
double RunLoadProfile(const std::vector<LoadSegment>& profile, int totalPlayers,
                      bool parallel, const MpParallel::ParallelConfig& config,
                      uint64_t* outBackoffs = nullptr,
                      size_t* outFinalThreshold = nullptr)
{
  PartOne partOne;
  NullSendTarget sendTarget;
  partOne.SetSendTarget(&sendTarget);

  if (parallel) {
    partOne.ConfigureParallelism(config);
  }

  const int side = static_cast<int>(
    std::ceil(std::sqrt(static_cast<double>(totalPlayers))));
  const float spacing = 3800.f / static_cast<float>(std::max(side - 1, 1));
  auto posX = [&](int i) { return static_cast<float>(i % side) * spacing; };
  auto posY = [&](int i) { return static_cast<float>(i / side) * spacing; };

  std::vector<uint32_t> idx(totalPlayers);
  for (int i = 0; i < totalPlayers; ++i) {
    DoConnect(partOne, static_cast<Networking::UserId>(i));
    const uint32_t formId = 0xff000000 + static_cast<uint32_t>(i);
    partOne.CreateActor(formId, { posX(i), posY(i), 0.f }, 0.f, 0x3c);
    partOne.SetUserActor(static_cast<Networking::UserId>(i), formId);
    idx[i] = dynamic_cast<MpActor*>(
               partOne.worldState.LookupFormById(formId).get())
               ->GetIdx();
  }

  auto oneTick = [&](int active) {
    for (int i = 0; i < active; ++i) {
      SendBinaryMovement(partOne, static_cast<Networking::UserId>(i), idx[i],
                         posX(i) + 1.f, posY(i) + 1.f);
    }
    partOne.Tick();
  };

  // Warm up at the profile's first level so allocator growth and subscription
  // setup are not charged to the measurement.
  for (int w = 0; w < 10; ++w) {
    oneTick(profile.empty() ? totalPlayers : profile.front().activePlayers);
  }

  const auto start = std::chrono::steady_clock::now();
  for (const LoadSegment& segment : profile) {
    for (int t = 0; t < segment.ticks; ++t) {
      oneTick(segment.activePlayers);
    }
  }
  const auto elapsed = std::chrono::steady_clock::now() - start;

  if (parallel) {
    const MpParallel::ParallelMetrics& m = partOne.GetParallelMetrics();
    if (outBackoffs) {
      *outBackoffs = m.totalAdaptiveBackoffs;
    }
    if (outFinalThreshold) {
      *outFinalThreshold = m.lastAdaptiveThreshold;
    }
  }
  return std::chrono::duration<double, std::micro>(elapsed).count();
}

TEST_CASE("Idle threads: does pool size cost anything on its own?",
          "[.][ParallelBench]")
{
  // The question any adaptive width control turns on. If the number of workers
  // *involved* in a tick is what matters, a controller can vary it per tick for
  // free by capping the shard count -- surplus workers stay parked in the
  // condition variable and cost nothing. If merely having the threads exist is
  // what costs, then adapting width means destroying and respawning OS threads,
  // which is far too expensive to do per tick.
  //
  // The unit count is pinned with maxShardsPerCluster so that only the pool
  // size varies. Read each row against the 4-worker row: flat means idle
  // threads are free.
  constexpr int kTicks = 150;

  for (int players : { 150, 400 }) {
    const Sample baseline = RunScenario(players, false, 0, kTicks);
    std::printf("\n  %d players, unit count pinned, inline baseline %.1f us\n\n",
                players, baseline.perTickMicros);
    std::printf("  %-10s %10s %10s %10s\n", "pool size", "4 units", "8 units",
                "16 units");
    std::printf("  %s\n", std::string(46, '-').c_str());

    for (size_t workers : { size_t(4), size_t(8), size_t(16), size_t(24) }) {
      if (workers > std::thread::hardware_concurrency()) {
        continue;
      }
      std::printf("  %-10zu", workers);
      for (size_t units : { size_t(4), size_t(8), size_t(16) }) {
        const Sample run =
          RunScenario(players, true, workers, kTicks, false, false, 1, units);
        REQUIRE(run.relays == baseline.relays);
        std::printf(" %9.1f", run.perTickMicros);
      }
      std::printf("\n");
    }
  }
  std::printf("\n");
}

TEST_CASE("A changing population moves the optimum", "[.][ParallelBench]")
{
  // The premise behind tuning anything at run time: no single fixed shard
  // budget is best across a population that moves. If this prints a single
  // column that wins every segment, adaptive tuning has nothing to chase and
  // the fixed defaults should stay.
  //
  // The profile is a raid forming and dispersing: quiet, ramp, packed, ramp
  // back down.
  const std::vector<LoadSegment> profile = {
    { 200, 40 },  { 150, 120 }, { 200, 300 },
    { 300, 400 }, { 150, 120 }, { 200, 40 },
  };

  int totalTicks = 0;
  for (const LoadSegment& s : profile) {
    totalTicks += s.ticks;
  }

  std::printf("\n  one raid cycle (%d ticks, 40 -> 400 -> 40 senders)\n\n",
              totalTicks);
  std::printf("  %-16s %12s %10s\n", "maxShards", "total ms", "us/tick");
  std::printf("  %s\n", std::string(42, '-').c_str());

  double best = 0.0;
  size_t bestShards = 0;
  for (size_t shards : { size_t(2), size_t(4), size_t(8), size_t(16),
                         size_t(32) }) {
    MpParallel::ParallelConfig config = MakeConfig(0);
    config.maxShardsPerCluster = shards;
    config.minShardMicros = 1;
    config.Normalize();

    const double total = RunLoadProfile(profile, 400, true, config);
    std::printf("  %-16zu %12.1f %10.1f\n", shards, total / 1000.0,
                total / totalTicks);
    if (best == 0.0 || total < best) {
      best = total;
      bestShards = shards;
    }
  }

  MpParallel::ParallelConfig off;
  off.enabled = false;
  const double inlineTotal = RunLoadProfile(profile, 400, false, off);
  std::printf("  %-16s %12.1f %10.1f\n", "inline", inlineTotal / 1000.0,
              inlineTotal / totalTicks);
  std::printf("\n  best fixed ceiling over the whole cycle: %zu shards\n\n",
              bestShards);

  REQUIRE(best > 0.0);
}

TEST_CASE("Adaptive threshold controller against a fixed one",
          "[.][ParallelBench]")
{
  // The controller exists to find the offload break-even on hardware nobody
  // benchmarked. The test of that claim is whether it lands near the fixed
  // setting that was measured here -- if it is worse than a constant on the
  // machine the constant was tuned for, it is not tuning, it is noise.
  constexpr int kTicks = 150;
  std::printf("\n  us/tick, adaptive threshold vs fixed\n\n");
  std::printf("  %-8s %10s %10s %10s %8s %9s %8s\n", "players", "inline",
              "fixed=1", "adaptive", "ratio", "backoffs", "thresh");
  std::printf("  %s\n", std::string(70, '-').c_str());

  for (int players : { 50, 100, 150, 250, 400 }) {
    const Sample baseline = RunScenario(players, false, 0, kTicks);

    const double fixed =
      RunLoadProfile({ { kTicks, players } }, players, true, MakeConfig(0)) /
      kTicks;

    // Reported so a losing row says *why* it lost: a backoff count above zero
    // means the controller switched the pool off and spent time in the
    // degraded path, which is the only way it can lose by much.
    uint64_t backoffs = 0;
    size_t threshold = 0;
    const double adaptive =
      RunLoadProfile({ { kTicks, players } }, players, true,
                     MakeAdaptiveConfig(0), &backoffs, &threshold) /
      kTicks;

    std::printf("  %-8d %10.1f %10.1f %10.1f %7.2fx %9llu %8zu\n", players,
                baseline.perTickMicros, fixed, adaptive, adaptive / fixed,
                static_cast<unsigned long long>(backoffs), threshold);
  }
  std::printf("\n  (>1.00x means the controller is losing to the constant)\n\n");
}

TEST_CASE("Work gate against a packed crowd", "[.][ParallelBench]")
{
  // The other half of the evidence for minOffloadWorkMicros. Its companion is
  // `Where the work gate should sit` in ParallelSimulation.cpp, which measures
  // a scattered population; this one measures the crowd the offload exists
  // for. A gate tuned only against the scattered case would switch the pool
  // off here, which is the one place it is clearly worth having.
  constexpr int kTicks = 150;
  std::printf("\n  packed crowd: us/tick by minOffloadWorkMicros\n\n");
  std::printf("  %-8s %9s", "players", "inline");
  for (uint64_t g : { 1ull, 50ull, 100ull, 150ull, 250ull, 500ull }) {
    std::printf(" %8llu", static_cast<unsigned long long>(g));
  }
  std::printf("\n  %s\n", std::string(66, '-').c_str());

  for (int players : { 100, 150, 250, 400 }) {
    const Sample baseline = RunScenario(players, false, 0, kTicks);
    std::printf("  %-8d %9.1f", players, baseline.perTickMicros);
    for (uint64_t gate : { 1ull, 50ull, 100ull, 150ull, 250ull, 500ull }) {
      MpParallel::ParallelConfig config = MakeConfig(0);
      config.minOffloadWorkMicros = gate;
      config.Normalize();
      const double total =
        RunLoadProfile({ { kTicks, players } }, players, true, config);
      std::printf(" %8.1f", total / kTicks);
    }
    std::printf("\n");
  }
  std::printf("\n  (1 is effectively no gate)\n\n");
}

TEST_CASE("Cost of a wrong offload threshold, in both directions",
          "[.][ParallelBench]")
{
  // minActorsToOffload is a break-even measured on one machine, so the useful
  // question for anything that tunes it is how much it costs to be wrong --
  // and whether the penalty is symmetric. If it is not, a controller should
  // be biased toward the cheap side rather than trying to sit exactly on the
  // boundary.
  constexpr int kTicks = 150;
  std::printf("\n  us/tick by minActorsToOffload\n\n");
  std::printf("  %-8s %10s", "players", "inline");
  for (size_t t : { size_t(1), size_t(100), size_t(300), size_t(100000) }) {
    std::printf(" %9zu", t);
  }
  std::printf("\n  %s\n", std::string(58, '-').c_str());

  for (int players : { 50, 100, 150, 250, 400 }) {
    const Sample baseline = RunScenario(players, false, 0, kTicks);
    std::printf("  %-8d %9.1f", players, baseline.perTickMicros);

    for (size_t threshold : { size_t(1), size_t(100), size_t(300),
                              size_t(100000) }) {
      MpParallel::ParallelConfig config = MakeConfig(0);
      config.minActorsToOffload = threshold;
      config.Normalize();
      const double total =
        RunLoadProfile({ { kTicks, players } }, players, true, config);
      std::printf(" %9.1f", total / kTicks);
    }
    std::printf("\n");
  }
  // 100000 is "never offload": the snapshot is still built and the relay
  // decisions still made, just on the calling thread. It isolates what the
  // feature costs when the pool is switched off but the machinery is not.
  std::printf("\n  (100000 = pool never engages; snapshot still built)\n\n");
}

TEST_CASE("Parallel offload scaling by worker count", "[.][ParallelBench]")
{
  // This is where the auto-detect default comes from. More workers is not
  // monotonically better: past the point where the shard budget can keep them
  // fed, extra threads only add scheduling pressure and cross-core traffic on
  // the output buffers the join has to read back.
  constexpr int kTicks = 150;

  for (int players : { 150, 400 }) {
    const Sample baseline = RunScenario(players, false, 0, kTicks);
    std::printf("\n  %d players, %d ticks, inline baseline = %.1f us/tick\n\n",
                players, kTicks, baseline.perTickMicros);
    std::printf("  %-8s %12s %9s %8s\n", "workers", "us/tick", "speedup",
                "units");
    std::printf("  %s\n", std::string(42, '-').c_str());

    for (size_t workers : { size_t(1), size_t(2), size_t(4), size_t(6),
                            size_t(8), size_t(12), size_t(16), size_t(24) }) {
      if (workers > std::thread::hardware_concurrency()) {
        continue;
      }
      const Sample run = RunScenario(players, true, workers, kTicks);
      REQUIRE(run.relays == baseline.relays);
      std::printf("  %-8zu %12.1f %8.2fx %8zu\n", workers, run.perTickMicros,
                  baseline.perTickMicros / run.perTickMicros, run.units);
    }
  }
  std::printf("\n");
}
