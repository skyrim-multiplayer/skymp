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
  void Send(Networking::UserId, Networking::PacketData, size_t, bool) override
  {
    ++sendCount;
  }
  uint64_t sendCount = 0;
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
};

// One stretch of a session: hold this many of the clients active for this
// long. Everyone stays connected throughout -- an inactive client is one
// whose player is not moving, which is the common case on a real server.
struct LoadPhase
{
  double seconds = 0.0;
  int activeClients = 0;
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

  // Flatten the phase list into a per-tick "how many are moving" schedule.
  std::vector<int> activeByTick;
  if (phases.empty()) {
    activeByTick.assign(static_cast<size_t>(simSeconds * tickHz), players);
  } else {
    for (const LoadPhase& phase : phases) {
      const auto n = static_cast<size_t>(phase.seconds * tickHz);
      activeByTick.insert(activeByTick.end(), n,
                          std::min(phase.activeClients, players));
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
      sendTarget.sendCount = 0;
      result.packetsDropped = 0;
    }

    // Clients emit on their own schedule, not the server's. During warm-up
    // the first phase's level is used, so the run starts settled.
    const int activeNow =
      activeByTick[static_cast<size_t>(std::max(tick, 0))];

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
        InFlightPacket p;
        p.arrivalMs = t + delay;
        p.clientIndex = ci;
        p.x = c.homeX + std::cos(c.wanderPhase) * c.wanderRadius;
        p.y = c.homeY + std::sin(c.wanderPhase) * c.wanderRadius;
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
    partOne.Tick();

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
    n > 0 ? sendTarget.sendCount / static_cast<uint64_t>(n) : 0;

  if (parallel) {
    const MpParallel::ParallelMetrics& m = partOne.GetParallelMetrics();
    result.backoffs = m.totalAdaptiveBackoffs;
    result.finalThreshold = m.lastAdaptiveThreshold;
    result.clusters = m.lastClusterCount;
  }
  return result;
}

MpParallel::ParallelConfig SimConfig(bool adaptive,
                                     uint64_t minOffloadWorkMicros = 0,
                                     int probeTicks = -1,
                                     float minSpeedup = -1.f)
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

  const SimResult r =
    RunSimulation(300, kSeconds, kTickHz, true, SimConfig(false), 20260805,
                  /*verbose=*/true);

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
  std::printf("  %-7s %-9s %9s %9s %9s %8s %7s\n", "players", "mode",
              "mean us", "p99 us", "movers", "backoff", "thresh");
  std::printf("  %s\n", std::string(66, '-').c_str());

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

    std::printf("  %-7d %-9s %9.1f %9.1f %9.1f %8s %7s\n", players, "inline",
                inl.meanTickMicros, inl.p99, inl.meanActorsPerTick, "-", "-");
    std::printf("  %-7s %-9s %9.1f %9.1f %9.1f %8s %7s\n", "", "fixed",
                fixed.meanTickMicros, fixed.p99, fixed.meanActorsPerTick, "-",
                "-");
    std::printf("  %-7s %-9s %9.1f %9.1f %9.1f %8llu %7zu\n", "", "adaptive",
                adaptive.meanTickMicros, adaptive.p99,
                adaptive.meanActorsPerTick,
                static_cast<unsigned long long>(adaptive.backoffs),
                adaptive.finalThreshold);

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
