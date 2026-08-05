#include "ParallelConfig.h"

#include <algorithm>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#if defined(_M_X64) || defined(_M_IX86)
#include <intrin.h>
#endif
#elif defined(__linux__)
#include <fstream>
#include <string>
#include <unordered_set>
#endif

namespace MpParallel {

namespace {

// Ceiling on the *auto-detected* worker count. An explicit workerThreads in
// server-settings.json is only bounded by kMaxWorkerThreads.
//
// This bounds two things at once, and the second is the one that matters.
//
// What actually decides the tick cost is how many workers a tick *involves*,
// which is the work-unit count, not how many threads exist. Measured by the
// `Idle threads` case on a 16-core/32-thread Ryzen 9950X3D at 400 players in
// one area, us/tick with the unit count pinned so only pool size varies:
//
//     pool size    4 units   8 units   16 units
//     4              622       610       615
//     8              628       544       551
//     16             628       547       583
//     24             627       550       572
//
// Down a column, 8 workers to 24 costs about 1%. Across a row, 4 units to 8 is
// worth 12%. Surplus threads park in the condition variable and are nearly
// free; at 150 players the residual is larger, around 10%, but still far below
// what the unit count is worth.
//
// So the cap earns its keep indirectly: the auto shard budget ceiling is
// `slots * 2`, so capping the pool at 8 caps the auto-sized unit count at 18,
// which measured at or near the optimum for every population tried. It also
// keeps the residual pool-size cost small on machines with many cores.
//
// The cap stays at 8, on measurement.
//
// It was briefly raised to 32, on the grounds that the 8 was an artefact of
// the wake-accounting bug and that "simulation and benchmarking on 16-core and
// AWS Graviton/Ice Lake systems" showed clean scaling past it. Neither holds:
// the 8 was measured *after* that bug was fixed, no Graviton or Ice Lake
// hardware was ever run, and perf_model.py -- the simulation in question --
// hardcodes `min(physical - 1, 8)` as its own worker rule, so it never modelled
// the change at all. Its per-platform IPC and barrier multipliers are
// hand-authored estimates, not calibrations.
//
// Re-measured on this machine, uncapping cost real time (us/tick, one chunk):
//
//     players            150     400
//     cap 8             150.6   615.4
//     cap 32            198.0   650.5
//
// At 400 players it raised the auto unit count from 18 to 31, which the
// `Idle threads` table already showed is the wrong direction.
//
// 8 is not a universal optimum and is not claimed to be one. It is the largest
// value with evidence behind it on the only hardware anyone has run. Operators
// on a bigger machine should run ParallelBenchmark and set workerThreads
// explicitly -- an explicit value is bounded only by kMaxWorkerThreads.
constexpr size_t kMaxAutoWorkerThreads = 8;

template <typename T>
T ReadNumber(const nlohmann::json& obj, const char* key, T fallback)
{
  auto it = obj.find(key);
  if (it == obj.end() || it->is_null()) {
    return fallback;
  }
  if (!it->is_number()) {
    throw std::runtime_error(
      fmt::format("parallelism.{} must be a number", key));
  }
  return it->get<T>();
}

bool ReadBool(const nlohmann::json& obj, const char* key, bool fallback)
{
  auto it = obj.find(key);
  if (it == obj.end() || it->is_null()) {
    return fallback;
  }
  if (!it->is_boolean()) {
    throw std::runtime_error(
      fmt::format("parallelism.{} must be a boolean", key));
  }
  return it->get<bool>();
}

size_t GetPhysicalCoreCount()
{
  size_t fallback = std::thread::hardware_concurrency();
  
#ifdef _WIN32
  DWORD length = 0;
  GetLogicalProcessorInformation(nullptr, &length);
  if (length == 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return fallback > 1 ? fallback / 2 : 1;
  }

  std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
    length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
  if (!GetLogicalProcessorInformation(buffer.data(), &length)) {
    return fallback > 1 ? fallback / 2 : 1;
  }

  size_t physicalCores = 0;
  for (const auto& info : buffer) {
    if (info.Relationship == RelationProcessorCore) {
      physicalCores++;
    }
  }
  return physicalCores > 0 ? physicalCores : (fallback > 1 ? fallback / 2 : 1);
#elif defined(__linux__)
  // Count distinct (physical id, core id) pairs in /proc/cpuinfo. Present on
  // x86, so SMT siblings collapse to one core each.
  std::ifstream cpuinfo("/proc/cpuinfo");
  if (!cpuinfo.is_open()) {
    return fallback > 1 ? fallback / 2 : 1;
  }
  std::unordered_set<std::string> cores;
  std::string line;
  std::string currentPhysicalId;
  std::string currentCoreId;
  size_t processorEntries = 0;

  const auto flush = [&] {
    if (!currentPhysicalId.empty() && !currentCoreId.empty()) {
      cores.insert(currentPhysicalId + "-" + currentCoreId);
    }
    currentPhysicalId.clear();
    currentCoreId.clear();
  };

  while (std::getline(cpuinfo, line)) {
    if (line.find("processor") == 0) {
      ++processorEntries;
    } else if (line.find("physical id") == 0) {
      const size_t pos = line.find(':');
      if (pos != std::string::npos) {
        currentPhysicalId = line.substr(pos + 1);
      }
    } else if (line.find("core id") == 0) {
      const size_t pos = line.find(':');
      if (pos != std::string::npos) {
        currentCoreId = line.substr(pos + 1);
      }
    } else if (line.empty()) {
      flush();
    }
  }
  // The last block is not followed by a blank line on every kernel.
  flush();

  if (!cores.empty()) {
    return cores.size();
  }

  // No topology fields. This is the normal shape of /proc/cpuinfo on aarch64
  // -- a Graviton instance lists `processor` entries and no `core id` at all
  // -- and those parts have no SMT, so every processor entry *is* a physical
  // core. Halving here, as the previous revision did, would have given a
  // 16-core Graviton 8 workers. Falling back to the processor count is right
  // wherever the topology fields are absent because there is no topology to
  // report; the halving fallback is kept only for the case where the file
  // could not be read at all and SMT cannot be ruled out.
  if (processorEntries > 0) {
    return processorEntries;
  }
  return fallback > 1 ? fallback / 2 : 1;
#else
  return fallback > 1 ? fallback / 2 : 1;
#endif
}

}

void ParallelConfig::Normalize()
{
  if (workerThreads == 0) {
    // Determine the actual number of physical cores via OS APIs to properly
    // support processors without HyperThreading, such as Intel E-cores or ARM.
    // One core is left for the Node/V8 thread that drives ScampServer::Tick.
    const size_t physical = GetPhysicalCoreCount();
    workerThreads = physical > 1 ? physical - 1 : 1;
    workerThreads = std::min(workerThreads, kMaxAutoWorkerThreads);
  }
  workerThreads = std::min(workerThreads, kMaxWorkerThreads);
  workerThreads = std::max<size_t>(workerThreads, 1);

  clusterSeparationChunks =
    std::max(clusterSeparationChunks, kMinSafeSeparationChunks);

  minClusterActors = std::max<size_t>(minClusterActors, 1);
  minActorsToOffload = std::max<size_t>(minActorsToOffload, 1);
  minShardActors = std::max<size_t>(minShardActors, 1);
  
  // No vendor branch here.
  //
  // A previous revision tripled minShardMicros on Intel parts, citing
  // "simulation and benchmarks show 55-95us is optimal for Ice Lake". No Intel
  // hardware was ever run; the figure comes from perf_model.py, whose Ice Lake
  // profile is a hand-written `ipc=0.75, barrier_scale=1.8` guess rather than
  // a calibration. Shipping a real behaviour change on a modelled constant is
  // how a server ends up slow for a reason nobody can reproduce.
  //
  // It was also unable to do what it claimed. The test `minShardMicros == 20`
  // cannot tell "the operator left the default" from "the operator measured
  // their hardware and chose 20", so it silently overrode explicit
  // configuration on every Intel host.
  //
  // The setting is already self-calibrating in the way that matters: the shard
  // budget divides *measured* per-actor cost by it, so a slower machine
  // naturally produces the same shard sizes in wall-clock terms. If a vendor
  // split turns out to be real, it needs a measurement on that vendor's
  // hardware first.
  minShardMicros = std::max<uint32_t>(minShardMicros, 1);

  // A spin longer than the tick period would keep every worker on a core for
  // the whole frame, which is the failure mode this is meant to avoid.
  workerSpinMicros = std::min<uint32_t>(workerSpinMicros, 5000);

  // Prevent division-by-zero in the adaptive decay modulo check.
  adaptiveDecayTicks = std::max<uint32_t>(adaptiveDecayTicks, 1);
  // Zero would make every noisy tick a backoff, which is the behaviour this
  // setting exists to prevent.
  adaptiveBackoffTicks = std::max<uint32_t>(adaptiveBackoffTicks, 1);
  // A bias below 1.0 would permanently disable offloading.
  adaptiveBias = std::max(adaptiveBias, 1.0f);
  adaptiveThresholdFloor = std::max<size_t>(adaptiveThresholdFloor, 1);

  if (targetTickBudgetMicros == 0) {
    targetTickBudgetMicros = 8000;
  }

  throttleDistanceUnits = std::max(throttleDistanceUnits, 1.f);
  maxThrottleSkipTicks = std::min<uint32_t>(maxThrottleSkipTicks, 32);

  interestFullRateUnits = std::max(interestFullRateUnits, 1.f);
  maxInterestSkipTicks =
    std::min<uint32_t>(std::max<uint32_t>(maxInterestSkipTicks, 1), 32);
}

ParallelConfig ParallelConfig::FromServerSettings(
  const nlohmann::json& serverSettings)
{
  ParallelConfig config;

  auto it = serverSettings.find("parallelism");
  if (it == serverSettings.end() || it->is_null()) {
    config.Normalize();
    return config;
  }

  if (!it->is_object()) {
    throw std::runtime_error("parallelism must be an object");
  }

  const nlohmann::json& j = *it;

  config.enabled = ReadBool(j, "enabled", config.enabled);
  config.adaptiveParallelism =
    ReadBool(j, "adaptiveParallelism", config.adaptiveParallelism);
  config.adaptiveBias =
    ReadNumber<float>(j, "adaptiveBias", config.adaptiveBias);
  config.adaptiveDecayTicks =
    ReadNumber<uint32_t>(j, "adaptiveDecayTicks", config.adaptiveDecayTicks);
  config.adaptiveBackoffTicks = ReadNumber<uint32_t>(
    j, "adaptiveBackoffTicks", config.adaptiveBackoffTicks);
  config.adaptiveCooldownTicks = ReadNumber<uint32_t>(
    j, "adaptiveCooldownTicks", config.adaptiveCooldownTicks);
  config.minOffloadWorkMicros = ReadNumber<uint64_t>(
    j, "minOffloadWorkMicros", config.minOffloadWorkMicros);
  config.adaptiveThresholdFloor =
    ReadNumber<size_t>(j, "adaptiveThresholdFloor", config.adaptiveThresholdFloor);
  config.adaptiveThrottling =
    ReadBool(j, "adaptiveThrottling", config.adaptiveThrottling);
  config.interestManagement =
    ReadBool(j, "interestManagement", config.interestManagement);
  config.interestFullRateUnits = ReadNumber<float>(
    j, "interestFullRateUnits", config.interestFullRateUnits);
  config.maxInterestSkipTicks = ReadNumber<uint32_t>(
    j, "maxInterestSkipTicks", config.maxInterestSkipTicks);

  config.workerThreads =
    ReadNumber<size_t>(j, "workerThreads", config.workerThreads);
  config.minActorsToOffload =
    ReadNumber<size_t>(j, "minActorsToOffload", config.minActorsToOffload);
  config.minClusterActors =
    ReadNumber<size_t>(j, "minClusterActors", config.minClusterActors);
  config.minShardActors =
    ReadNumber<size_t>(j, "minShardActors", config.minShardActors);
  config.maxShardsPerCluster =
    ReadNumber<size_t>(j, "maxShardsPerCluster", config.maxShardsPerCluster);
  config.clusterSeparationChunks = ReadNumber<int32_t>(
    j, "clusterSeparationChunks", config.clusterSeparationChunks);
  config.maxWorkUnitsPerTick =
    ReadNumber<size_t>(j, "maxWorkUnitsPerTick", config.maxWorkUnitsPerTick);
  config.minShardMicros =
    ReadNumber<uint32_t>(j, "minShardMicros", config.minShardMicros);
  config.workerSpinMicros =
    ReadNumber<uint32_t>(j, "workerSpinMicros", config.workerSpinMicros);
  config.targetTickBudgetMicros = ReadNumber<uint64_t>(
    j, "targetTickBudgetMicros", config.targetTickBudgetMicros);
  config.throttleDistanceUnits =
    ReadNumber<float>(j, "throttleDistanceUnits", config.throttleDistanceUnits);
  config.maxThrottleSkipTicks =
    ReadNumber<uint32_t>(j, "maxThrottleSkipTicks", config.maxThrottleSkipTicks);
  config.metricsLogIntervalTicks = ReadNumber<uint32_t>(
    j, "metricsLogIntervalTicks", config.metricsLogIntervalTicks);

  config.Normalize();
  return config;
}

std::string ParallelConfig::Describe() const
{
  if (!enabled) {
    return "parallel area offload: disabled";
  }
  return fmt::format(
    "parallel area offload: enabled, workerThreads={}, "
    "minActorsToOffload={}, adaptiveParallelism={}, minClusterActors={}, minShardActors={}, "
    "minShardMicros={}, spin={}us, separation={} chunks, "
    "interestManagement={} (fullRate={}u, maxSkip={}), "
    "adaptiveThrottling={}, budget={}us",
    workerThreads, minActorsToOffload, adaptiveParallelism ? "on" : "off", minClusterActors, minShardActors,
    minShardMicros, workerSpinMicros, clusterSeparationChunks,
    interestManagement ? "on" : "off", interestFullRateUnits,
    maxInterestSkipTicks, adaptiveThrottling ? "on" : "off",
    targetTickBudgetMicros);
}

}
