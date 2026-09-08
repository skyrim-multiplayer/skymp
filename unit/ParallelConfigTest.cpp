#include "parallel/LoadBalancer.h"
#include "parallel/ParallelConfig.h"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>
#include <thread>

using namespace MpParallel;

TEST_CASE("Parallelism is off unless asked for", "[ParallelConfig]")
{
  const nlohmann::json settings = nlohmann::json::object();
  const ParallelConfig config = ParallelConfig::FromServerSettings(settings);

  REQUIRE_FALSE(config.enabled);
  REQUIRE(config.Describe() == "parallel area offload: disabled");
}

TEST_CASE("Settings are read and clamped", "[ParallelConfig]")
{
  nlohmann::json settings;
  settings["parallelism"] = {
    { "enabled", true },
    { "workerThreads", 6 },
    { "minActorsToOffload", 40 },
    { "minClusterActors", 0 },          // clamped up to 1
    { "clusterSeparationChunks", 1 },   // clamped up to the safe minimum
    { "targetTickBudgetMicros", 0 },    // replaced by the default
    { "maxThrottleSkipTicks", 9999 },   // clamped down
    { "throttleDistanceUnits", 2048.0 },
  };

  const ParallelConfig config = ParallelConfig::FromServerSettings(settings);

  REQUIRE(config.enabled);
  REQUIRE(config.workerThreads == 6);
  REQUIRE(config.minActorsToOffload == 40);
  REQUIRE(config.minClusterActors == 1);
  REQUIRE(config.clusterSeparationChunks == kMinSafeSeparationChunks);
  REQUIRE(config.targetTickBudgetMicros == 8000);
  REQUIRE(config.maxThrottleSkipTicks == 32);
  REQUIRE(config.throttleDistanceUnits == 2048.f);
}

TEST_CASE("Worker count of zero resolves to something usable",
          "[ParallelConfig]")
{
  nlohmann::json settings;
  settings["parallelism"] = { { "enabled", true }, { "workerThreads", 0 } };

  const ParallelConfig config = ParallelConfig::FromServerSettings(settings);

  REQUIRE(config.workerThreads >= 1);
  REQUIRE(config.workerThreads <= kMaxWorkerThreads);
}

TEST_CASE("An oversized worker count is capped", "[ParallelConfig]")
{
  nlohmann::json settings;
  settings["parallelism"] = { { "enabled", true },
                              { "workerThreads", 100000 } };

  const ParallelConfig config = ParallelConfig::FromServerSettings(settings);
  REQUIRE(config.workerThreads == kMaxWorkerThreads);
}

TEST_CASE("Normalize is idempotent", "[ParallelConfig]")
{
  ParallelConfig config;
  config.enabled = true;
  config.Normalize();
  const ParallelConfig once = config;
  config.Normalize();

  REQUIRE(config.workerThreads == once.workerThreads);
  REQUIRE(config.clusterSeparationChunks == once.clusterSeparationChunks);
  REQUIRE(config.targetTickBudgetMicros == once.targetTickBudgetMicros);
}

TEST_CASE("Malformed settings are rejected loudly", "[ParallelConfig]")
{
  SECTION("Wrong type for the whole block")
  {
    nlohmann::json settings;
    settings["parallelism"] = "yes please";
    REQUIRE_THROWS(ParallelConfig::FromServerSettings(settings));
  }

  SECTION("Wrong type for a field")
  {
    nlohmann::json settings;
    settings["parallelism"] = { { "workerThreads", "many" } };
    REQUIRE_THROWS(ParallelConfig::FromServerSettings(settings));
  }

  SECTION("Wrong type for a boolean")
  {
    nlohmann::json settings;
    settings["parallelism"] = { { "enabled", 1 } };
    REQUIRE_THROWS(ParallelConfig::FromServerSettings(settings));
  }

  SECTION("Null means default")
  {
    nlohmann::json settings;
    settings["parallelism"] = nullptr;
    REQUIRE_NOTHROW(ParallelConfig::FromServerSettings(settings));
  }
}

TEST_CASE("ParallelConfig: LoadBalancer estimates linearly", "[ParallelConfig]") {
  LoadBalancer balancer;

  AreaCluster small;
  small.representative = AreaKey{ 0x3c, 0, 0 };
  small.actorIndices.assign(2, 0);

  AreaCluster large;
  large.representative = AreaKey{ 0x3c, 50, 50 };
  large.actorIndices.assign(40, 0);

  REQUIRE(balancer.EstimateMicros(large) > balancer.EstimateMicros(small));
}

TEST_CASE("ParallelConfig: LoadBalancer tracks moving average",
          "[ParallelConfig]") {
  LoadBalancer balancer;

  AreaCluster cluster;
  cluster.representative = AreaKey{ 0x3c, 1, 1 };
  cluster.actorIndices.assign(1, 0);

  // Repeated identical samples must converge on the observed value.
  for (int i = 0; i < 40; ++i) {
    balancer.Observe(cluster.representative, 5000, static_cast<uint64_t>(i));
  }

  const uint64_t estimate = balancer.EstimateMicros(cluster);
  REQUIRE(estimate > 4500);
  REQUIRE(estimate <= 5000);
}

TEST_CASE("The schedule puts expensive clusters first", "[ParallelBalancer]")
{
  LoadBalancer balancer;

  std::vector<AreaCluster> clusters(3);
  clusters[0].representative = AreaKey{ 0x3c, 0, 0 };
  clusters[1].representative = AreaKey{ 0x3c, 10, 0 };
  clusters[2].representative = AreaKey{ 0x3c, 20, 0 };

  balancer.Observe(clusters[0].representative, 100, 1);
  balancer.Observe(clusters[1].representative, 9000, 1);
  balancer.Observe(clusters[2].representative, 3000, 1);

  std::vector<uint32_t> order;
  balancer.BuildSchedule(clusters, order);

  REQUIRE(order == std::vector<uint32_t>{ 1, 2, 0 });
}

TEST_CASE("Pressure rises only when an area exceeds its budget",
          "[ParallelBalancer]")
{
  LoadBalancer balancer;
  const AreaKey key{ 0x3c, 4, 4 };

  SECTION("Nothing measured means no pressure")
  {
    REQUIRE(balancer.GetPressureLevel(key, 1000, 4) == 0);
  }

  SECTION("Comfortably inside the budget means no pressure")
  {
    for (int i = 0; i < 20; ++i) {
      balancer.Observe(key, 200, static_cast<uint64_t>(i));
    }
    REQUIRE(balancer.GetPressureLevel(key, 1000, 4) == 0);
  }

  SECTION("Well over the budget raises pressure, capped at maxLevel")
  {
    for (int i = 0; i < 40; ++i) {
      balancer.Observe(key, 100000, static_cast<uint64_t>(i));
    }
    const uint32_t pressure = balancer.GetPressureLevel(key, 1000, 4);
    REQUIRE(pressure > 0);
    REQUIRE(pressure <= 4);
  }

  SECTION("A single slow tick does not by itself start throttling")
  {
    for (int i = 0; i < 40; ++i) {
      balancer.Observe(key, 100, static_cast<uint64_t>(i));
    }
    // One tick ten times slower than usual, e.g. a stall elsewhere in the
    // process. Smoothing has to absorb it rather than punish players.
    balancer.Observe(key, 1000, 41);
    REQUIRE(balancer.GetPressureLevel(key, 500, 8) == 0);

    // Sustained overload, on the other hand, must be noticed.
    for (int i = 42; i < 80; ++i) {
      balancer.Observe(key, 5000, static_cast<uint64_t>(i));
    }
    REQUIRE(balancer.GetPressureLevel(key, 500, 8) > 0);
  }
}

TEST_CASE("Stale areas are evicted", "[ParallelBalancer]")
{
  LoadBalancer balancer;
  balancer.Observe(AreaKey{ 0x3c, 0, 0 }, 500, 10);
  balancer.Observe(AreaKey{ 0x3c, 9, 9 }, 500, 900);
  REQUIRE(balancer.GetTrackedAreaCount() == 2);

  balancer.EvictStale(1000, 500);
  REQUIRE(balancer.GetTrackedAreaCount() == 1);

  balancer.Clear();
  REQUIRE(balancer.GetTrackedAreaCount() == 0);
}
