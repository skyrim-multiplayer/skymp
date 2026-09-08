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
  config.Normalize();
  return config;
}

Sample RunScenario(int players, bool parallel, size_t workers, int ticks,
                   bool useJson = false, bool interestMgmt = false)
{
  PartOne partOne;
  NullSendTarget sendTarget;
  partOne.SetSendTarget(&sendTarget);

  if (parallel) {
    MpParallel::ParallelConfig config = MakeConfig(workers);
    config.interestManagement = interestMgmt;
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
  const auto start = std::chrono::steady_clock::now();
  for (int t = 0; t < ticks; ++t) {
    oneTick();
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
    sample.parallelMicros = m.lastParallelMicros;
    sample.joinMicros = m.lastJoinMicros;
    sample.aggregateTaskMicros = m.lastAggregateTaskMicros;
  }
  return sample;
}

}

TEST_CASE("Parallel offload throughput vs inline", "[.][ParallelBench]")
{
  // Extended past 150 to find the crossover: the offload only earns its
  // barrier overhead once the parallel phase is large enough to amortize it.
  const std::vector<int> populations = { 25, 50, 100, 150, 250, 400 };
  constexpr int kTicks = 40;

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

    // The crowd must have formed one cluster and been sharded, otherwise the
    // benchmark is not measuring what it claims to.
    REQUIRE(parallelRun.clusters == 1);
    REQUIRE(parallelRun.units > 1);
  }
  std::printf("\n");
}

TEST_CASE("Interest management: what cutting relays actually buys",
          "[.][ParallelBench]")
{
  // At 400 players the join emits 160k relays serially and dominates the
  // tick. Parallelising decisions cannot help that; sending less can. This
  // measures the only lever that attacks the N^2 term directly.
  constexpr int kTicks = 30;
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

TEST_CASE("Parallel offload scaling by worker count", "[.][ParallelBench]")
{
  constexpr int kPlayers = 150;
  constexpr int kTicks = 40;

  const Sample baseline = RunScenario(kPlayers, false, 0, kTicks);
  std::printf("\n  %d players, %d ticks, inline baseline = %.1f us/tick\n\n",
              kPlayers, kTicks, baseline.perTickMicros);
  std::printf("  %-8s %12s %9s\n", "workers", "us/tick", "speedup");
  std::printf("  %s\n", std::string(34, '-').c_str());

  for (size_t workers : { size_t(1), size_t(2), size_t(4), size_t(8),
                          size_t(16) }) {
    if (workers > std::thread::hardware_concurrency()) {
      continue;
    }
    const Sample run = RunScenario(kPlayers, true, workers, kTicks);
    REQUIRE(run.relays == baseline.relays);
    std::printf("  %-8zu %12.1f %8.2fx\n", workers, run.perTickMicros,
                baseline.perTickMicros / run.perTickMicros);
  }
  std::printf("\n");
}
