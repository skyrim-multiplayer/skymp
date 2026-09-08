#include "TestUtils.hpp"
#include "MessageSerializerFactory.h"
#include "UpdateMovementMessage.h"
#include "parallel/OffloadDispatcher.h"
#include "parallel/ParallelConfig.h"
#include "parallel/ParallelMetrics.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <queue>
#include <random>
#include <slikenet/BitStream.h>
#include <vector>

// A population of clients that do not all look alike, driving the real server.
//
// WHAT THIS SIMULATES, AND WHAT IT DOES NOT
//
// It varies the *client* population: how fast each machine sends, how long its
// packets take to arrive, how much that time varies, how often a packet is
// lost, and whereabouts in the world its player stands. All of that is input
// to the server, and it is what the offload-threshold controller keys on -- so
// running it against the real PartOne, the real packet parser and the real
// dispatcher is a genuine test of the controller.
//
// It does NOT simulate a different *server* CPU. Running an "AWS Graviton
// profile" on this host would produce timings from this host and a label from
// somewhere else, which is precisely the mistake that put a fabricated
// justification into ParallelConfig.cpp -- a Python model with hand-authored
// per-platform IPC and barrier multipliers was cited as "benchmarking on
// Graviton/Ice Lake systems". Numbers here are this machine's. A claim about
// other server hardware still needs that hardware.
//
// What this fills in that the other benchmarks do not:
//
//   * Every existing case puts every player in one chunk, so the partitioner
//     always produces exactly one cluster. Real populations are spread across
//     a city, its outskirts, open country and interiors, which is what makes
//     clustering do anything at all.
//
//   * Every existing case has every player send exactly once per tick, so the
//     per-tick actor count is a constant. Real clients send at their own frame
//     rate, over links with latency and jitter, so the count fluctuates tick
//     to tick -- and that count is the exact signal the controller reads.
//
// Hidden behind "[.]" so ctest ignores it:
//
//   ./unit/unit "[ParallelSim]"

namespace {

class CountingSendTarget : public Networking::ISendTarget
{
public:
  CountingSendTarget() { ResetSendCount(); }

  void Send(Networking::UserId, Networking::PacketData, size_t, bool) override
  {
    thread_local size_t local_idx = []() {
      static std::atomic<size_t> next_idx{0};
      return next_idx.fetch_add(1, std::memory_order_relaxed) % 64;
    }();
    counts[local_idx].count.fetch_add(1, std::memory_order_relaxed);
  }

  struct alignas(64) PaddedCount {
    std::atomic<uint64_t> count{0};
  };
  PaddedCount counts[64];

  uint64_t GetSendCount() const {
    uint64_t total = 0;
    for (int i = 0; i < 64; ++i) {
      total += counts[i].count.load(std::memory_order_relaxed);
    }
    return total;
  }

  void ResetSendCount() {
    for (int i = 0; i < 64; ++i) {
      counts[i].count.store(0, std::memory_order_relaxed);
    }
  }
};

// How fast a client's machine produces movement updates. SkyMP sends one per
// rendered frame, so this is really a frame-rate distribution.
struct HardwareTier
{
  const char* name;
  float sendHz;
  int weight; // relative share of the population
};

// Latency and its variability. Only the client-to-server leg matters here,
// since that is what decides which tick an update lands on.
struct NetworkTier
{
  const char* name;
  float oneWayMs;
  float jitterMs;
  float lossPct;
  int weight;
};

// Where in the world a player stands. This is what decides how many clusters
// the partitioner finds and how much work each sender's relay set is.
struct LocaleTier
{
  const char* name;
  // Radius, in world units, of the blob this player is placed in.
  float spreadUnits;
  // Number of distinct blobs of this kind. Whiterun is one city; wilderness is
  // many scattered spots.
  int blobCount;
  // A tight blob far from anything else -- a dungeon mouth, a mine, a camp.
  //
  // These stay in Tamriel rather than getting their own interior cell id.
  // FormDesc::FromFormId special-cases 0x3c and otherwise indexes the load
  // order, which is empty in this harness, so a synthetic cell id throws
  // "invalid file index 0". Separate-grid behaviour -- that two different
  // worldOrCells can never share a cluster -- is already asserted directly by
  // ParallelPartitionerTest, so nothing is lost by keeping one worldspace
  // here; the clustering this file exercises is the spatial kind.
  bool tightBlob;
  int weight;
};

const std::vector<HardwareTier> kHardware = {
  { "potato laptop", 30.f, 20 },  { "steam deck", 40.f, 15 },
  { "mid desktop", 60.f, 45 },    { "high refresh", 144.f, 20 },
};

const std::vector<NetworkTier> kNetwork = {
  { "fibre, in region", 10.f, 2.f, 0.00f, 30 },
  { "cable", 30.f, 8.f, 0.10f, 35 },
  { "congested wifi", 40.f, 35.f, 1.00f, 15 },
  { "transatlantic", 70.f, 15.f, 0.30f, 12 },
  { "mobile", 60.f, 70.f, 2.00f, 8 },
};

const std::vector<LocaleTier> kLocale = {
  { "city centre", 1200.f, 1, false, 40 },
  { "city outskirts", 4000.f, 3, false, 25 },
  { "open country", 40000.f, 12, false, 25 },
  { "dungeon/camp", 600.f, 8, true, 10 },
};

struct Client
{
  Networking::UserId userId = 0;
  uint32_t formId = 0;
  uint32_t idx = 0;

  float sendIntervalMs = 16.f;
  float nextSendMs = 0.f;

  float oneWayMs = 10.f;
  float jitterMs = 2.f;
  float lossPct = 0.f;

  float homeX = 0.f;
  float homeY = 0.f;
  uint32_t worldOrCell = 0x3c;

  // Small wander so grid positions genuinely change over the run, which is
  // what makes the partitioner re-cluster rather than reuse a static answer.
  float wanderPhase = 0.f;
  float wanderRadius = 0.f;

  const char* hardwareName = "";
  const char* networkName = "";
  const char* localeName = "";
};

// Where a gathered population converges. Arbitrary, but fixed, so the same
// seed produces the same crowd.
constexpr float kGatherX = 1500.f;
constexpr float kGatherY = 1500.f;

struct InFlightPacket
{
  float arrivalMs = 0.f;
  size_t clientIndex = 0;
  float x = 0.f;
  float y = 0.f;

  // Ordered by arrival time; std::priority_queue is a max-heap, so invert.
  bool operator<(const InFlightPacket& rhs) const
  {
    return arrivalMs > rhs.arrivalMs;
  }
};

template <typename T>
const T& PickWeighted(const std::vector<T>& tiers, std::mt19937& rng)
{
  int total = 0;
  for (const T& tier : tiers) {
    total += tier.weight;
  }
  std::uniform_int_distribution<int> dist(0, total - 1);
  int roll = dist(rng);
  for (const T& tier : tiers) {
    roll -= tier.weight;
    if (roll < 0) {
      return tier;
    }
  }
  return tiers.back();
}

void SendBinaryMovement(PartOne& partOne, Networking::UserId userId,
                        uint32_t idx, uint32_t worldOrCell, float x, float y)
{
  UpdateMovementMessage msg;
  msg.idx = idx;
  msg.data.worldOrCell = worldOrCell;
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

struct SimResult
{
  double meanTickMicros = 0.0;
  double p50 = 0.0;
  double p99 = 0.0;
  double worst = 0.0;

  double meanActorsPerTick = 0.0;
  size_t maxActorsInATick = 0;
  size_t clusters = 0;

  uint64_t relaysPerTick = 0;
  uint64_t backoffs = 0;
  size_t finalThreshold = 0;
  uint64_t packetsDropped = 0;
  uint64_t packetsDelivered = 0;

  // What the gate is deciding on, and what it decided.
  double achievedSpeedup = 0.0;
  double declinedFraction = 0.0;

  // The two halves of the profitability question: how much work the parallel
  // phase had, and what it cost in wall clock. Savings is the difference.
  uint64_t taskMicros = 0;
  uint64_t parallelMicros = 0;
  uint64_t joinMicros = 0;

  // What the last paired trial measured for each path, and which it kept.
  double trialAccept = 0.0;
  double trialDecline = 0.0;
  bool trialVerdictAccept = true;
  uint64_t trials = 0;

  // Relay edges the rate policy suppressed. Zero means neither interest
  // management nor the pressure throttle did anything at all.
  uint64_t throttledPerTick = 0;

  // Submissions dropped at join time because the actor's owner disconnected
  // between ingest and tick. Non-zero confirms the stale-actor guards fire.
  uint64_t staleActors = 0;
};

// One stretch of a session: hold this many of the clients active for this
// long. Everyone stays connected throughout -- an inactive client is one
// whose player is not moving, which is the common case on a real server.
struct LoadPhase
{
  double seconds = 0.0;
  int activeClients = 0;

  // How far the population has migrated toward a single gathering point, from
  // 0 (everyone at home, scattered across the province) to 1 (everyone in one
  // square).
  //
  // This is the axis the rest of the harness was missing. Varying how many
  // players are moving changes the head count, which the gate tracks on every
  // tick from attempts alone. Varying *where they are* changes density, which
  // nothing observes unless the dispatcher takes a tick on and measures it --
  // and that is the whole reason adaptiveProbeIntervalTicks exists. Without
  // this, that setting had no test.
  float gather = 0.f;

  // Fraction of active clients that disconnect and reconnect each tick.
  // At 0.05 with 300 active movers, ~15 disconnect per tick. The disconnect
  // happens after movement packets are delivered but before Tick(), which is
  // the exact race the stale-actor guards in PartOneOffloadSink exist for:
  // movement was submitted for a user who is gone by join time.
  float churnRate = 0.f;

  // If true, destroy one active actor per tick after movement is delivered
  // but before Tick(). This tests the other half of the race: the actor
  // form itself is unloaded/destroyed before the join can apply movement to it.
  bool destroyOneActorMidIngest = false;
};

// Builds a mixed population and runs it against the real server for
// `simSeconds` of simulated time at `tickHz`.
//
// When `phases` is non-empty it overrides simSeconds and drives a population
// that changes over time.
SimResult RunSimulation(int players, double simSeconds, float tickHz,
                        bool parallel,
                        const MpParallel::ParallelConfig& config,
                        uint32_t seed, bool verbose = false,
                        const std::vector<LoadPhase>& phases = {})
{
  std::mt19937 rng(seed);

  PartOne partOne;
  CountingSendTarget sendTarget;
  partOne.SetSendTarget(&sendTarget);
  if (parallel) {
    partOne.ConfigureParallelism(config);
  }

  // Blob centres, laid out far enough apart that separate localities really do
  // land in separate chunks.
  std::uniform_real_distribution<float> blobDist(-120000.f, 120000.f);
  std::vector<std::vector<std::pair<float, float>>> blobCentres(kLocale.size());
  for (size_t i = 0; i < kLocale.size(); ++i) {
    for (int b = 0; b < kLocale[i].blobCount; ++b) {
      blobCentres[i].push_back({ blobDist(rng), blobDist(rng) });
    }
  }

  std::vector<Client> clients(players);
  std::uniform_real_distribution<float> unit(0.f, 1.f);
  std::uniform_real_distribution<float> angle(0.f, 6.2831853f);

  for (int i = 0; i < players; ++i) {
    Client& c = clients[i];
    c.userId = static_cast<Networking::UserId>(i);
    c.formId = 0xff000000 + static_cast<uint32_t>(i);

    const HardwareTier& hw = PickWeighted(kHardware, rng);
    const NetworkTier& net = PickWeighted(kNetwork, rng);

    size_t localeIndex = 0;
    {
      int total = 0;
      for (const LocaleTier& t : kLocale) {
        total += t.weight;
      }
      std::uniform_int_distribution<int> dist(0, total - 1);
      int roll = dist(rng);
      for (size_t li = 0; li < kLocale.size(); ++li) {
        roll -= kLocale[li].weight;
        if (roll < 0) {
          localeIndex = li;
          break;
        }
      }
    }
    const LocaleTier& loc = kLocale[localeIndex];

    c.sendIntervalMs = 1000.f / hw.sendHz;
    // Stagger first sends so the population does not start in lockstep.
    c.nextSendMs = unit(rng) * c.sendIntervalMs;
    c.oneWayMs = net.oneWayMs;
    c.jitterMs = net.jitterMs;
    c.lossPct = net.lossPct;
    c.hardwareName = hw.name;
    c.networkName = net.name;
    c.localeName = loc.name;

    const auto& centres = blobCentres[localeIndex];
    const auto& centre = centres[rng() % centres.size()];
    const float r = std::sqrt(unit(rng)) * loc.spreadUnits;
    const float a = angle(rng);
    c.homeX = centre.first + r * std::cos(a);
    c.homeY = centre.second + r * std::sin(a);

    // Tamriel for everyone; see the note on LocaleTier::tightBlob.
    c.worldOrCell = 0x3c;

    c.wanderPhase = angle(rng);
    c.wanderRadius = loc.tightBlob ? 200.f : 900.f;

    DoConnect(partOne, c.userId);
    partOne.CreateActor(c.formId, { c.homeX, c.homeY, 0.f }, 0.f,
                        c.worldOrCell);
    partOne.SetUserActor(c.userId, c.formId);
    c.idx = dynamic_cast<MpActor*>(
              partOne.worldState.LookupFormById(c.formId).get())
              ->GetIdx();
  }

  if (verbose) {
    std::printf("\n  population of %d:\n", players);
    for (const HardwareTier& hw : kHardware) {
      const auto n = std::count_if(
        clients.begin(), clients.end(),
        [&](const Client& c) { return c.hardwareName == hw.name; });
      std::printf("    %-16s %4zu clients @ %.0f Hz\n", hw.name,
                  static_cast<size_t>(n), hw.sendHz);
    }
    for (const NetworkTier& net : kNetwork) {
      const auto n = std::count_if(
        clients.begin(), clients.end(),
        [&](const Client& c) { return c.networkName == net.name; });
      std::printf("    %-16s %4zu clients, %.0fms +/- %.0fms, %.1f%% loss\n",
                  net.name, static_cast<size_t>(n), net.oneWayMs, net.jitterMs,
                  net.lossPct);
    }
    for (const LocaleTier& loc : kLocale) {
      const auto n = std::count_if(
        clients.begin(), clients.end(),
        [&](const Client& c) { return c.localeName == loc.name; });
      std::printf("    %-16s %4zu clients\n", loc.name,
                  static_cast<size_t>(n));
    }
  }

  const float tickMs = 1000.f / tickHz;

  // Flatten the phase list into per-tick schedules of "how many are moving",
  // "how gathered they are", and "how much churn".
  std::vector<int> activeByTick;
  std::vector<float> gatherByTick;
  std::vector<float> churnByTick;
  std::vector<bool> destroyByTick;
  if (phases.empty()) {
    activeByTick.assign(static_cast<size_t>(simSeconds * tickHz), players);
    gatherByTick.assign(activeByTick.size(), 0.f);
    churnByTick.assign(activeByTick.size(), 0.f);
    destroyByTick.assign(activeByTick.size(), false);
  } else {
    for (size_t p = 0; p < phases.size(); ++p) {
      const LoadPhase& phase = phases[p];
      const auto n = static_cast<size_t>(phase.seconds * tickHz);
      const float from = p == 0 ? phase.gather : phases[p - 1].gather;
      for (size_t i = 0; i < n; ++i) {
        activeByTick.push_back(std::min(phase.activeClients, players));
        // Ramp across the phase rather than teleporting the population, so
        // the partition changes at something like walking pace.
        const float t = n > 1 ? static_cast<float>(i) / static_cast<float>(n - 1)
                              : 1.f;
        gatherByTick.push_back(from + (phase.gather - from) * t);
        churnByTick.push_back(phase.churnRate);
        destroyByTick.push_back(phase.destroyOneActorMidIngest);
      }
    }
  }
  const int totalTicks = static_cast<int>(activeByTick.size());

  std::priority_queue<InFlightPacket> inFlight;
  std::normal_distribution<float> jitterDist(0.f, 1.f);
  std::uniform_real_distribution<float> lossRoll(0.f, 100.f);

  SimResult result;
  std::vector<double> tickMicros;
  tickMicros.reserve(totalTicks);
  std::vector<size_t> actorsPerTick;
  actorsPerTick.reserve(totalTicks);

  float nowMs = 0.f;

  // Warm up: subscriptions, allocator growth, and the first partition.
  const int warmupTicks = static_cast<int>(tickHz);

  for (int tick = -warmupTicks; tick < totalTicks; ++tick) {
    if (tick == 0) {
      // Warm-up relays would otherwise be divided by the measured tick count.
      sendTarget.ResetSendCount();
      result.packetsDropped = 0;
    }

    // Clients emit on their own schedule, not the server's. During warm-up
    // the first phase's level is used, so the run starts settled.
    const size_t schedIndex = static_cast<size_t>(std::max(tick, 0));
    const int activeNow = activeByTick[schedIndex];
    const float gatherNow = gatherByTick[schedIndex];
    const float churnNow = churnByTick[schedIndex];
    const bool destroyNow = destroyByTick[schedIndex];

    for (size_t ci = 0; ci < clients.size(); ++ci) {
      Client& c = clients[ci];
      if (static_cast<int>(ci) >= activeNow) {
        // Not moving this phase. Keep its send clock rolling so it does not
        // burst a backlog the moment it becomes active again.
        while (c.nextSendMs <= nowMs + tickMs) {
          c.nextSendMs += c.sendIntervalMs;
        }
        continue;
      }
      while (c.nextSendMs <= nowMs + tickMs) {
        const float t = c.nextSendMs;
        c.nextSendMs += c.sendIntervalMs;

        if (lossRoll(rng) < c.lossPct) {
          ++result.packetsDropped;
          continue;
        }

        // Latency plus jitter, floored so a packet never arrives before it
        // was sent.
        const float delay =
          std::max(0.f, c.oneWayMs + jitterDist(rng) * c.jitterMs);

        c.wanderPhase += 0.05f;
        // Interpolate toward the gathering point. At gather = 1 the whole
        // population is inside one chunk, which is the packed case the offload
        // exists for, reached without changing how many players are moving.
        const float gx = c.homeX + (kGatherX - c.homeX) * gatherNow;
        const float gy = c.homeY + (kGatherY - c.homeY) * gatherNow;
        InFlightPacket p;
        p.arrivalMs = t + delay;
        p.clientIndex = ci;
        p.x = gx + std::cos(c.wanderPhase) * c.wanderRadius;
        p.y = gy + std::sin(c.wanderPhase) * c.wanderRadius;
        inFlight.push(p);
      }
    }

    nowMs += tickMs;

    // Deliver everything that has arrived, then tick. Timing covers ingest and
    // the tick together, which is the same unit of work the other benchmarks
    // measure.
    const auto start = std::chrono::steady_clock::now();

    size_t deliveredThisTick = 0;
    while (!inFlight.empty() && inFlight.top().arrivalMs <= nowMs) {
      const InFlightPacket p = inFlight.top();
      inFlight.pop();
      const Client& c = clients[p.clientIndex];
      SendBinaryMovement(partOne, c.userId, c.idx, c.worldOrCell, p.x, p.y);
      ++deliveredThisTick;
    }

    // --- mid-ingest churn ---------------------------------------------------
    // Disconnect a random subset of active clients *after* their movement
    // packets have been delivered but *before* Tick(). This is the exact race
    // the stale-actor guards exist for: movement was submitted for a user who
    // is gone by the time the join runs. (Or the actor form itself is
    // destroyed, which simulates unloading).
    //
    // Disconnected/destroyed clients are repaired on the next tick so the
    // population size stays roughly constant, which isolates the effect from
    // the head-count effect.
    std::vector<size_t> disconnectedThisTick;
    std::vector<size_t> destroyedThisTick;
    if (churnNow > 0.f && tick >= 0) {
      std::uniform_real_distribution<float> churnRoll(0.f, 1.f);
      for (size_t ci = 0; ci < clients.size(); ++ci) {
        if (static_cast<int>(ci) >= activeNow) {
          continue;
        }
        if (churnRoll(rng) < churnNow) {
          DoDisconnect(partOne, clients[ci].userId);
          disconnectedThisTick.push_back(ci);
        }
      }
    }
    if (destroyNow && tick >= 0 && activeNow > 0) {
      // Pick one active client and destroy their actor.
      std::uniform_int_distribution<size_t> pick(0, activeNow - 1);
      const size_t ci = pick(rng);
      partOne.DestroyActor(clients[ci].formId);
      destroyedThisTick.push_back(ci);
    }

    partOne.Tick();

    // Reconnect anyone who was churned out, before the next tick's ingest.
    for (size_t ci : disconnectedThisTick) {
      Client& c = clients[ci];
      DoConnect(partOne, c.userId);
      partOne.SetUserActor(c.userId, c.formId);
    }
    for (size_t ci : destroyedThisTick) {
      Client& c = clients[ci];
      // Generate a new formId and recreate the actor, restoring the mapping
      c.formId = 0xff000000 | static_cast<uint32_t>(c.userId);
      partOne.CreateActor(c.formId, { c.homeX, c.homeY, 0.f }, 0.f,
                          c.worldOrCell, 0);
      partOne.SetUserActor(c.userId, c.formId);
    }

    const auto elapsed = std::chrono::steady_clock::now() - start;

    if (tick >= 0) {
      tickMicros.push_back(
        std::chrono::duration<double, std::micro>(elapsed).count());
      actorsPerTick.push_back(deliveredThisTick);
      result.packetsDelivered += deliveredThisTick;
    }
  }

  std::sort(tickMicros.begin(), tickMicros.end());
  const auto n = tickMicros.size();
  if (n > 0) {
    result.meanTickMicros =
      std::accumulate(tickMicros.begin(), tickMicros.end(), 0.0) / n;
    result.p50 = tickMicros[n / 2];
    result.p99 = tickMicros[std::min(n - 1, (n * 99) / 100)];
    result.worst = tickMicros.back();
  }
  if (!actorsPerTick.empty()) {
    result.meanActorsPerTick =
      static_cast<double>(std::accumulate(actorsPerTick.begin(),
                                          actorsPerTick.end(), size_t(0))) /
      actorsPerTick.size();
    result.maxActorsInATick =
      *std::max_element(actorsPerTick.begin(), actorsPerTick.end());
  }
  result.relaysPerTick =
    n > 0 ? sendTarget.GetSendCount() / static_cast<uint64_t>(n) : 0;

  if (parallel) {
    const MpParallel::ParallelMetrics& m = partOne.GetParallelMetrics();
    result.backoffs = m.totalTrials;
    result.finalThreshold = m.lastAttemptCount;
    result.clusters = m.lastClusterCount;
    result.achievedSpeedup = m.lastAchievedSpeedup;
    result.taskMicros = m.lastAggregateTaskMicros;
    result.trialAccept = m.lastTrialAcceptMicrosPerMover;
    result.trialDecline = m.lastTrialDeclineMicrosPerMover;
    result.trialVerdictAccept = m.lastTrialAccepted;
    result.trials = m.totalTrials;
    result.throttledPerTick = m.totalTicks > 0
      ? m.totalRelayEdgesThrottled / m.totalTicks
      : 0;
    result.parallelMicros = m.lastParallelMicros;
    result.joinMicros = m.lastJoinMicros;
    result.declinedFraction = m.totalTicks > 0
      ? static_cast<double>(m.totalDeclinedTicks) /
          static_cast<double>(m.totalTicks)
      : 0.0;
    result.staleActors = m.totalStaleActors;
  }
  return result;
}

MpParallel::ParallelConfig SimConfig(bool adaptive,
                                     uint64_t minOffloadWorkMicros = 0,
                                     int probeTicks = -1,
                                     float minSpeedup = -1.f,
                                     int trialIntervalTicks = -1,
                                     bool throttling = false,
                                     uint64_t tickBudgetMicros = 0,
                                     float throttleDistance = 0.f)
{
  MpParallel::ParallelConfig config;
  config.enabled = true;
  config.workerThreads = 0;
  config.minClusterActors = 1;
  config.minShardActors = 2;
  config.adaptiveThrottling = false;
  config.interestManagement = true;
  config.adaptiveParallelism = adaptive;
  // 0 keeps the shipped default; anything else is the sweep pinning it.
  if (minOffloadWorkMicros > 0) {
    config.minOffloadWorkMicros = minOffloadWorkMicros;
  }
  // -1 keeps the shipped default; 0 disables probing entirely.
  if (probeTicks >= 0) {
    config.adaptiveProbeIntervalTicks = static_cast<uint32_t>(probeTicks);
  }
  // Negative keeps the shipped default; 0 disables the speedup gate.
  if (minSpeedup >= 0.f) {
    config.minOffloadSpeedup = minSpeedup;
  }
  // The shipped trial interval is 1800 ticks -- thirty seconds -- which is
  // longer than most cases here run for, so the trial would never complete and
  // the mechanism would go unmeasured. Cases that care pass a shorter one.
  if (trialIntervalTicks >= 0) {
    config.abTrialIntervalTicks = static_cast<uint32_t>(trialIntervalTicks);
  }
  config.adaptiveThrottling = throttling;
  if (tickBudgetMicros > 0) {
    config.targetTickBudgetMicros = tickBudgetMicros;
  }
  if (throttleDistance > 0.f) {
    config.throttleDistanceUnits = throttleDistance;
  }
  config.Normalize();
  return config;
}

}

TEST_CASE("Simulated mixed population: what the world actually looks like",
          "[.][ParallelSim]")
{
  // Establishes the shape of the workload before anything is tuned against it.
  // The number that matters here is the cluster count: every other benchmark
  // in this suite produces exactly one, and a controller tuned only against
  // that has never seen the case it will actually meet.
  constexpr double kSeconds = 6.0;
  constexpr float kTickHz = 60.f;

  // Gates off. This case describes the *workload* -- how many clusters a
  // realistic population forms, how much the per-tick mover count moves -- and
  // with the shipped gates the dispatcher would rightly decline nearly every
  // tick of it, leaving nothing to describe. What the gate decides about this
  // workload is the next case's business.
  const SimResult r =
    RunSimulation(300, kSeconds, kTickHz, true, SimConfig(false, 1, -1, 0.f),
                  20260805, /*verbose=*/true);

  std::printf("\n  %g simulated seconds at %.0f Hz\n", kSeconds, kTickHz);
  std::printf("    movers per tick   mean %.1f, peak %zu\n",
              r.meanActorsPerTick, r.maxActorsInATick);
  std::printf("    clusters          %zu\n", r.clusters);
  std::printf("    relays per tick   %llu\n",
              static_cast<unsigned long long>(r.relaysPerTick));
  std::printf("    packets           %llu delivered, %llu lost\n",
              static_cast<unsigned long long>(r.packetsDelivered),
              static_cast<unsigned long long>(r.packetsDropped));
  std::printf("    tick us           mean %.1f  p50 %.1f  p99 %.1f  worst %.1f\n\n",
              r.meanTickMicros, r.p50, r.p99, r.worst);

  // A mixed population must not collapse to the single-chunk case, or this
  // file is measuring the same thing as everything else.
  REQUIRE(r.clusters > 1);
  // Clients sending at their own frame rates over jittery links must produce a
  // per-tick count that actually varies.
  REQUIRE(r.maxActorsInATick > static_cast<size_t>(r.meanActorsPerTick));
}

TEST_CASE("A raid forming while the server is on the inline path",
          "[.][ParallelSim]")
{
  // The scenario the decline gate is most likely to get wrong.
  //
  // While declining, no work is submitted, so nothing measures how expensive a
  // tick would have been. If the estimate that drives the decision is built
  // only from ticks that were accepted, it freezes the moment the server
  // starts declining -- and then a population that grows underneath it can
  // never reopen the gate. The server would sit on the inline path through the
  // whole raid, which is exactly when the offload is worth having.
  //
  // Quiet, then a raid forms and holds, then it disperses -- three times over,
  // because a single 13-second cycle swung by 15% run to run, which is wider
  // than the effect being measured. Three cycles and a median over seeds
  // brings it inside a couple of percent.
  std::vector<LoadPhase> raid;
  for (int cycle = 0; cycle < 3; ++cycle) {
    raid.push_back({ 3.0, 60 });
    raid.push_back({ 1.0, 200 });
    raid.push_back({ 5.0, 500 });
    raid.push_back({ 1.0, 200 });
    raid.push_back({ 3.0, 60 });
  }

  const std::vector<uint32_t> seeds = { 20260805, 991, 4242 };

  // Configurations are interleaved *within* a seed, and the comparison is a
  // ratio taken inside that seed, because the absolute numbers drift.
  //
  // This machine is a workstation, not a quiet bench: with a game and a chat
  // client running, the inline baseline for this case measured anywhere from
  // 429us to 542us between invocations. Running all seeds of one configuration
  // and then all seeds of the next puts the two in different halves of that
  // drift and silently attributes it to the change. Comparing configurations
  // measured seconds apart under the same conditions does not.
  std::vector<double> inlineMeans, gatedMeans, alwaysMeans;
  std::vector<double> inlineP99, gatedP99, alwaysP99;
  std::vector<double> gatedOverAlways, gatedOverInline;

  for (uint32_t seed : seeds) {
    const SimResult inl = RunSimulation(500, 0.0, 60.f, false,
                                        SimConfig(false), seed, false, raid);
    const SimResult gated = RunSimulation(500, 0.0, 60.f, true,
                                          SimConfig(false), seed, false, raid);
    // No gate at all: always takes the work on. The comparison that says
    // whether the gate is costing us the raid.
    const SimResult always = RunSimulation(500, 0.0, 60.f, true,
                                           SimConfig(false, 1, -1, 0.f), seed, false,
                                           raid);

    inlineMeans.push_back(inl.meanTickMicros);
    gatedMeans.push_back(gated.meanTickMicros);
    alwaysMeans.push_back(always.meanTickMicros);
    inlineP99.push_back(inl.p99);
    gatedP99.push_back(gated.p99);
    alwaysP99.push_back(always.p99);
    gatedOverAlways.push_back(gated.meanTickMicros / always.meanTickMicros);
    gatedOverInline.push_back(gated.meanTickMicros / inl.meanTickMicros);
  }

  auto median = [](std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
  };

  std::printf("\n  raid cycle x3: 60 -> 500 -> 60 movers, median of %zu seeds\n\n",
              seeds.size());
  std::printf("  %-24s %10s %10s\n", "configuration", "mean us", "p99 us");
  std::printf("  %s\n", std::string(48, '-').c_str());
  std::printf("  %-24s %10.1f %10.1f\n", "inline", median(inlineMeans),
              median(inlineP99));
  std::printf("  %-24s %10.1f %10.1f\n", "gated (default)", median(gatedMeans),
              median(gatedP99));
  std::printf("  %-24s %10.1f %10.1f\n", "never declines", median(alwaysMeans),
              median(alwaysP99));
  std::printf("\n  within-seed ratios: gated/never-declines %.2fx, "
              "gated/inline %.2fx\n\n",
              median(gatedOverAlways), median(gatedOverInline));

  // The gate must not turn a raid into a worse tick than never having gated.
  // If it latches shut when the population grows, this is where it shows: the
  // server spends the whole crowded phase on the inline path, and the ratio
  // was 1.07 before the attempt count and the probe were added.
  REQUIRE(median(gatedOverAlways) <= 1.05);
}

TEST_CASE("What the staleness probe costs", "[.][ParallelSim]")
{
  // The probe is the price of not latching: while the gate is shut it accepts
  // one tick per interval to re-measure, and on a population the gate is right
  // to be declining, that tick is pure loss. This is what that insurance
  // premium actually costs, so it can be traded against how long the gate is
  // allowed to be wrong when a raid forms.
  //
  // Expect roughly (cost of an accepted tick - cost of a declined one) divided
  // by the interval: at 100 players that is about (119 - 69) / 60, near 1%.
  // A number far above that would mean the probe is firing more often than it
  // should.
  constexpr double kSeconds = 6.0;
  constexpr float kTickHz = 60.f;
  const std::vector<uint32_t> seeds = { 20260805, 991, 4242 };

  std::printf("\n  mean us/tick by adaptiveProbeIntervalTicks\n\n");
  std::printf("  %-8s %9s %9s", "players", "inline", "no probe");
  for (int p : { 240, 60, 20 }) {
    std::printf(" %8d", p);
  }
  std::printf("\n  %s\n", std::string(58, '-').c_str());

  auto median = [&](bool parallel, int probe) {
    std::vector<double> means;
    for (uint32_t seed : seeds) {
      means.push_back(RunSimulation(200, kSeconds, kTickHz, parallel,
                                    SimConfig(false, 0, probe), seed)
                        .meanTickMicros);
    }
    std::sort(means.begin(), means.end());
    return means[means.size() / 2];
  };

  for (int players : { 100, 200 }) {
    auto medianFor = [&](bool parallel, int probe) {
      std::vector<double> means;
      for (uint32_t seed : seeds) {
        means.push_back(RunSimulation(players, kSeconds, kTickHz, parallel,
                                      SimConfig(false, 0, probe), seed)
                          .meanTickMicros);
      }
      std::sort(means.begin(), means.end());
      return means[means.size() / 2];
    };

    std::printf("  %-8d %9.1f %9.1f", players, medianFor(false, -1),
                medianFor(true, 0));
    for (int p : { 240, 60, 20 }) {
      std::printf(" %8.1f", medianFor(true, p));
    }
    std::printf("\n");
  }
  (void)median;
  std::printf("\n  (no probe = the gate can latch; see the raid case)\n\n");
}

TEST_CASE("Where the work gate should sit, spread population",
          "[.][ParallelSim]")
{
  // Half of the evidence for minOffloadWorkMicros. The other half is the
  // `Work gate against a packed crowd` case in ParallelBenchmark.cpp, and the
  // default has to be right for both -- a value tuned only against a scattered
  // population would switch the pool off on exactly the crowd it exists for.
  //
  // Each row: at what work estimate does engaging the pool stop being a loss?
  constexpr double kSeconds = 6.0;
  constexpr float kTickHz = 60.f;

  std::printf("\n  spread population: mean us/tick by minOffloadWorkMicros\n\n");
  std::printf("  %-8s %9s", "players", "inline");
  for (uint64_t g : { 0ull, 50ull, 100ull, 150ull, 250ull, 500ull }) {
    std::printf(" %8llu", static_cast<unsigned long long>(g));
  }
  std::printf("\n  %s\n", std::string(66, '-').c_str());

  for (int players : { 100, 200, 300, 500 }) {
    const SimResult inl = RunSimulation(players, kSeconds, kTickHz, false,
                                        SimConfig(false), 20260805);
    std::printf("  %-8d %9.1f", players, inl.meanTickMicros);
    for (uint64_t gate : { 0ull, 50ull, 100ull, 150ull, 250ull, 500ull }) {
      // 0 means "no work gate at all", i.e. the old head-count-only behaviour.
      const SimResult r =
        RunSimulation(players, kSeconds, kTickHz, true,
                      SimConfig(false, gate == 0 ? 1 : gate, -1,
                                gate == 0 ? 0.f : -1.f),
                      20260805);
      std::printf(" %8.1f", r.meanTickMicros);
    }
    std::printf("\n");
  }
  std::printf("\n  (first column after inline is the gate disabled)\n\n");
}

TEST_CASE("Controller against a mixed population, by size",
          "[.][ParallelSim]")
{
  // The question the controller exists to answer: on a population it was not
  // tuned against, does it land near the right setting, and does it stay
  // there? p99 is reported next to the mean because a controller that
  // oscillates buys an average and pays for it in the worst ticks, which is
  // the half a player notices.
  constexpr double kSeconds = 6.0;
  constexpr float kTickHz = 60.f;

  std::printf("\n  mixed population, adaptive vs fixed threshold\n\n");
  std::printf("  %-7s %-9s %8s %8s %7s %7s %8s %6s %7s\n", "players", "mode",
              "mean us", "p99 us", "movers", "speedup", "declined", "work",
              "us/act");
  std::printf("  %s\n", std::string(80, '-').c_str());

  // Median over seeds. A single 6-second run of this swung by 25% between
  // invocations -- far wider than the differences being reported -- so a
  // single-seed table here would be a coin toss dressed up as a measurement.
  const std::vector<uint32_t> seeds = { 20260805, 991, 4242 };

  auto median = [](std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
  };

  for (int players : { 100, 200, 300, 500 }) {
    // Interleaved within each seed so machine drift cancels; see the note in
    // the raid case.
    std::vector<SimResult> inls, fixeds, adaptives;
    std::vector<double> adaptiveOverInline;
    for (uint32_t seed : seeds) {
      inls.push_back(
        RunSimulation(players, kSeconds, kTickHz, false, SimConfig(false), seed));
      fixeds.push_back(
        RunSimulation(players, kSeconds, kTickHz, true, SimConfig(false), seed));
      // Shipped defaults, trial cadence included. Six seconds is 360 ticks and
      // the trial interval is 1800, so no trial completes here and this row
      // shows the fallback -- which is what a server looks like for its first
      // half minute. Forcing a short interval instead would make half these
      // ticks trial ticks and measure the harness rather than the server.
      // `What the paired trial concludes` is what exercises the mechanism.
      adaptives.push_back(
        RunSimulation(players, kSeconds, kTickHz, true, SimConfig(true), seed));
      adaptiveOverInline.push_back(adaptives.back().meanTickMicros /
                                   inls.back().meanTickMicros);
    }
    auto pick = [&](std::vector<SimResult>& v) {
      std::sort(v.begin(), v.end(),
                [](const SimResult& a, const SimResult& b) {
                  return a.meanTickMicros < b.meanTickMicros;
                });
      return v[v.size() / 2];
    };
    const SimResult inl = pick(inls);
    const SimResult fixed = pick(fixeds);
    const SimResult adaptive = pick(adaptives);

    // `work` and `us/act` are here because achieved speedup on its own turned
    // out not to separate the two workloads -- see the note on the assertion
    // below -- and these are the quantities the replacement test will need.
    auto row = [](const char* label, const SimResult& r, bool gated) {
      const double perActor = r.meanActorsPerTick > 0.0
        ? static_cast<double>(r.taskMicros) / r.meanActorsPerTick
        : 0.0;
      std::printf("  %-7s %-9s %8.1f %8.1f %7.1f", "", label,
                  r.meanTickMicros, r.p99, r.meanActorsPerTick);
      if (gated) {
        std::printf(" %7.2f %7.0f%% %6llu %7.2f\n", r.achievedSpeedup,
                    r.declinedFraction * 100.0,
                    static_cast<unsigned long long>(r.taskMicros), perActor);
      } else {
        std::printf(" %7s %8s %6s %7s\n", "-", "-", "-", "-");
      }
    };
    std::printf("  %-7d\n", players);
    row("inline", inl, false);
    row("fixed", fixed, true);
    row("adaptive", adaptive, true);

    // The gate must not make things worse than never having engaged at all.
    // Judged on the within-seed ratio, and generously: what this guards
    // against is the gate being *systematically* wrong -- before it existed
    // the offload cost 74% at 100 players -- not a few percent of workstation
    // noise.
    //
    // The bound is 1.25 rather than something tighter because of a real
    // effect, not just noise. On a machine with other work on it -- this one
    // was measured with a game and a chat client running, at 59% CPU -- the
    // offloaded path degrades harder than the inline one, because it wants
    // several cores where inline wants one. At 300 players that showed up as
    // 1.20x here while the same case measured 1.01x on a quiet machine.
    //
    // Worse, the gate currently leans the wrong way into it: the work estimate
    // is in absolute microseconds, so a contended (or simply slower) machine
    // makes the same population look like *more* work and opens the gate
    // wider, exactly when the pool has fewer cores to give. Fixing that means
    // gating on achieved parallel speedup rather than on absolute work, which
    // is a separate change and wants a quiet machine to calibrate on.
    REQUIRE(median(adaptiveOverInline) <= 1.25);
  }
  std::printf("\n");
}

TEST_CASE("What the paired trial concludes", "[.][ParallelSim]")
{
  // The mechanism on its own, separated from how often it runs.
  //
  // The aggregate timings in the case above cannot show this: to observe a
  // verdict's effect you need many coasting ticks, and to produce a verdict you
  // need trials, so any run short enough to inspect is dominated by trialling.
  // What matters is whether the trial reaches the *right* conclusion, and that
  // is these two numbers and the choice between them.
  //
  // Correct answers, from the aggregate tables: decline at 100, 200 and 300
  // spread players, accept at 500.
  constexpr double kSeconds = 12.0;
  constexpr float kTickHz = 60.f;

  std::printf("\n  paired trial: measured cost per mover on each path\n\n");
  std::printf("  %-8s %10s %10s %10s %8s\n", "players", "accept us",
              "decline us", "verdict", "trials");
  std::printf("  %s\n", std::string(52, '-').c_str());

  for (int players : { 100, 200, 300, 500 }) {
    const SimResult r =
      RunSimulation(players, kSeconds, kTickHz, true,
                    SimConfig(true, 0, -1, -1.f, 120), 20260805);
    std::printf("  %-8d %10.3f %10.3f %10s %8llu\n", players, r.trialAccept,
                r.trialDecline, r.trialVerdictAccept ? "accept" : "decline",
                static_cast<unsigned long long>(r.trials));

    // A trial must actually have run, or the rest of the row means nothing.
    REQUIRE(r.trials > 0);
    REQUIRE(r.trialAccept > 0.0);
    REQUIRE(r.trialDecline > 0.0);
  }
  std::printf("\n");
}

TEST_CASE("A crowd gathering without the head count changing",
          "[.][ParallelSim]")
{
  // The case adaptiveProbeIntervalTicks exists for, and until now had no test.
  //
  // The number of players moving is constant throughout. What changes is where
  // they are: a province-wide scatter walks in to a single square and back out
  // again. That is invisible to the attempt count -- the same 300 updates
  // arrive every tick either way -- so the only way the dispatcher can notice
  // is by taking a tick on and measuring it, which is exactly what the probe
  // and the trial do.
  //
  // If they did not, a server would sit on the inline path through a city
  // gathering, which is the single worst time to be there.
  constexpr float kTickHz = 60.f;
  const std::vector<LoadPhase> gathering = {
    { 4.0, 300, 0.f },   // scattered
    { 3.0, 300, 1.f },   // walking in
    { 6.0, 300, 1.f },   // packed
    { 3.0, 300, 0.f },   // dispersing
    { 4.0, 300, 0.f },   // scattered again
  };

  std::printf("\n  300 movers throughout; only their spread changes\n\n");
  std::printf("  %-22s %10s %10s %9s\n", "configuration", "mean us", "p99 us",
              "declined");
  std::printf("  %s\n", std::string(56, '-').c_str());

  auto run = [&](bool parallel, int probeTicks, const char* label) {
    const SimResult r =
      RunSimulation(300, 0.0, kTickHz, parallel,
                    SimConfig(true, 0, probeTicks, -1.f, 300), 20260805, false,
                    gathering);
    std::printf("  %-22s %10.1f %10.1f %8.0f%%\n", label, r.meanTickMicros,
                r.p99, r.declinedFraction * 100.0);
    return r;
  };

  const SimResult inl =
    RunSimulation(300, 0.0, kTickHz, false, SimConfig(false), 20260805, false,
                  gathering);
  std::printf("  %-22s %10.1f %10.1f %9s\n", "inline", inl.meanTickMicros,
              inl.p99, "-");

  const SimResult probing = run(true, -1, "probe on (default)");
  const SimResult blind = run(true, 0, "probe off");

  std::printf("\n");

  // With the probe off the dispatcher cannot see the crowd form, so it holds
  // whatever verdict the scattered phase produced. With it on it should do at
  // least as well.
  REQUIRE(probing.meanTickMicros <= blind.meanTickMicros * 1.10);
  // And the whole point: noticing the crowd must beat never engaging.
  REQUIRE(probing.meanTickMicros <= inl.meanTickMicros * 1.10);
}

TEST_CASE("Does the pressure throttle ever engage?", "[.][ParallelSim]")
{
  // adaptiveThrottling is the one mechanism that exists today to defend the
  // tick when a crowd forms, and every benchmark and simulation in this repo
  // had it switched off -- including this file. So it has never been measured
  // end to end, only its policy function unit-tested in isolation.
  //
  // The scenario is the one that hurts: 300 players walking into a single
  // square, where p99 reaches several milliseconds against a tick budget of
  // about one. If the throttle is going to earn its keep anywhere, here is
  // where.
  //
  // `throttled` is the number the case turns on. It counts relay edges the
  // rate policy suppressed, so a zero means the mechanism did not fire at all,
  // whatever the timings happen to say.
  constexpr float kTickHz = 60.f;
  const std::vector<LoadPhase> gathering = {
    { 3.0, 300, 0.f }, { 3.0, 300, 1.f }, { 6.0, 300, 1.f }, { 3.0, 300, 0.f },
  };

  std::printf("\n  300 movers walking into one square\n\n");
  std::printf("  %-34s %9s %9s %10s\n", "configuration", "mean us", "p99 us",
              "throttled");
  std::printf("  %s\n", std::string(66, '-').c_str());

  auto run = [&](const char* label, bool throttling, uint64_t budget,
                 float throttleDistance = 0.f) {
    // Gates off: this case is about the rate policy, not about which path the
    // work takes, and letting the gate decline half the ticks would hide it.
    MpParallel::ParallelConfig config =
      SimConfig(false, 1, -1, 0.f, -1, throttling, budget, throttleDistance);
    config.interestManagement = true;
    config.Normalize();

    const SimResult r = RunSimulation(300, 0.0, kTickHz, true, config,
                                      20260805, false, gathering);
    std::printf("  %-34s %9.1f %9.1f %10llu\n", label, r.meanTickMicros, r.p99,
                static_cast<unsigned long long>(r.throttledPerTick));
    return r;
  };

  const SimResult off = run("interest mgmt only", false, 0);
  const SimResult shipped = run("+ throttle, shipped budget 2000us", true, 0);
  const SimResult tight = run("+ throttle, budget 1000us", true, 1000);
  const SimResult tighter = run("+ throttle, budget 250us", true, 250);
  // The diagnosis. throttleDistanceUnits exempts anyone closer than it from
  // pressure throttling, and it defaults to 4096 -- one whole chunk. A crowd
  // packed into one chunk is, by definition, almost entirely inside that
  // exemption, so the throttle cannot reach the only situation it exists for.
  const SimResult reach =
    run("+ throttle, budget 250us, reach 400u", true, 250, 400.f);
  std::printf("\n");

  // Interest management alone must be doing something, or the baseline is
  // wrong rather than the throttle.
  REQUIRE(off.throttledPerTick > 0);

  // The regression this case was written to catch. Two separate defaults
  // conspired to make the throttle dead code:
  //
  //   * throttleDistanceUnits exempted anyone within 4096 units -- a whole
  //     chunk -- from pressure throttling, so a crowd packed into one chunk
  //     was entirely inside the exemption.
  //   * targetTickBudgetMicros was 8000, eight times what a tick has to give,
  //     so no area was ever judged under pressure in the first place.
  //
  // Either alone was enough to suppress zero edges at every setting tried.
  // With both fixed -- the exemption now halves per pressure level, and the
  // budget is 2000 -- the mechanism engages on a crowd, which is the only
  // place it was ever meant to.
  REQUIRE(shipped.throttledPerTick > off.throttledPerTick);
  REQUIRE(tight.throttledPerTick > off.throttledPerTick);
  REQUIRE(tighter.throttledPerTick > off.throttledPerTick);
  REQUIRE(reach.throttledPerTick > off.throttledPerTick);

  // Reducing the rate must never silence anyone: the relay count stays well
  // above zero however hard the throttle is pushed.
  REQUIRE(reach.relaysPerTick > 0);
}

TEST_CASE("Mid-ingest connect/disconnect churn", "[.][ParallelSim]")
{
  // The race the stale-actor guards exist for, tested end to end for the
  // first time.
  //
  // Movement is submitted for a user who then disconnects before Tick() runs.
  // By the time the join calls ApplyMovement, the actor's owner is gone. The
  // sink's ResolveActor / IsConnectedFast guards must catch this and skip the
  // submission rather than crashing or writing to freed memory.
  //
  // churnRate = 0.05 means ~5% of active clients disconnect every tick, which
  // is far higher than any real server sees. That is the point: if the guards
  // survive this they survive anything.
  constexpr float kTickHz = 60.f;

  // Packed crowd so the offload path is actually taken — a scattered
  // population would be declined and the race would never arise.
  const std::vector<LoadPhase> churn = {
    { 2.0, 300, 1.f, 0.f },    // settle, packed, no churn
    { 6.0, 300, 1.f, 0.05f },  // 5% churn per tick
    { 2.0, 300, 1.f, 0.f },    // settle again
  };

  std::printf("\n  300 packed movers, 5%% disconnect between ingest and tick\n\n");
  std::printf("  %-22s %10s %10s %9s %10s\n", "configuration", "mean us",
              "p99 us", "declined", "stale");
  std::printf("  %s\n", std::string(66, '-').c_str());

  auto run = [&](bool parallel, const char* label) {
    // Gates off so the offload path is always taken, which is where the race
    // lives. Interest management on so the relay list is built.
    MpParallel::ParallelConfig config =
      SimConfig(false, 1, -1, 0.f, -1, false, 0, 0.f);
    config.interestManagement = true;
    config.Normalize();

    const SimResult r = RunSimulation(300, 0.0, kTickHz, parallel, config,
                                      20260805, false, churn);
    std::printf("  %-22s %10.1f %10.1f %8.0f%% %10llu\n", label,
                r.meanTickMicros, r.p99, r.declinedFraction * 100.0,
                static_cast<unsigned long long>(r.staleActors));
    return r;
  };

  const SimResult inl =
    RunSimulation(300, 0.0, kTickHz, false, SimConfig(false), 20260805, false,
                  churn);
  std::printf("  %-22s %10.1f %10.1f %9s %10s\n", "inline", inl.meanTickMicros,
              inl.p99, "-", "-");

  const SimResult offloaded = run(true, "offloaded + churn");

  std::printf("\n");

  // Disconnecting doesn't destroy the actor form, so ResolveActor still
  // succeeds and the movement is applied safely. The stale count only ticks
  // when the form itself is gone.
  //
  // To verify that guard, we'll explicitly destroy some actor forms between
  // ingest and tick in the next case. For this case, surviving without
  // crashing is the success condition.
  REQUIRE(offloaded.staleActors == 0);

  // And the server must not have crashed or produced garbage timings.
  REQUIRE(offloaded.meanTickMicros > 0.0);
  REQUIRE(offloaded.p99 > 0.0);

  // The inline path gets no stale count (it's not measured there), but it
  // must also survive churn without crashing.
  REQUIRE(inl.meanTickMicros > 0.0);
}

TEST_CASE("Mid-ingest actor unload", "[.][ParallelSim]")
{
  constexpr float kTickHz = 60.f;

  const std::vector<LoadPhase> phases = {
    { 2.0, 300, 0.f, 0.f, false },    // settle
    { 2.0, 300, 0.f, 0.f, true },     // one actor destroyed per tick
  };

  std::printf("\n  300 packed movers, one destroyed mid-ingest\n\n");

  MpParallel::ParallelConfig config = SimConfig(false, 1, -1, 0.f, -1, false, 0, 0.f);
  config.interestManagement = true;
  config.Normalize();

  const SimResult r = RunSimulation(300, 0.0, kTickHz, true, config, 20260805, true, phases);

  std::printf("  %-22s %10.1f %10.1f %8.0f%% %10llu\n", "offloaded + destroy",
              r.meanTickMicros, r.p99, r.declinedFraction * 100.0,
              static_cast<unsigned long long>(r.staleActors));

  std::printf("\n");

  // Since we explicitly destroy 1 actor per tick during the 2-second phase
  // (120 ticks), and the crowd is packed so they all submit movement, we
  // expect roughly 120 stale actor drops (some might be lost to lossPct, but
  // well above 0).
  REQUIRE(r.staleActors > 0);
  REQUIRE(r.meanTickMicros > 0.0);
}
