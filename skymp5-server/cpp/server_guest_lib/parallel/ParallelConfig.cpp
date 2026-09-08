#include "ParallelConfig.h"

#include <algorithm>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <thread>

namespace MpParallel {

namespace {

// Number of cores deliberately left to other threads: the Node/V8 main
// thread that drives ScampServer::Tick, and Viet's async save-storage
// thread. Oversubscribing them is what turns a throughput win into a
// latency regression.
constexpr size_t kReservedCores = 2;

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

}

void ParallelConfig::Normalize()
{
  if (workerThreads == 0) {
    const size_t detected = std::thread::hardware_concurrency();
    workerThreads = detected > kReservedCores ? detected - kReservedCores : 1;
  }
  workerThreads = std::min(workerThreads, kMaxWorkerThreads);
  workerThreads = std::max<size_t>(workerThreads, 1);

  clusterSeparationChunks =
    std::max(clusterSeparationChunks, kMinSafeSeparationChunks);

  minClusterActors = std::max<size_t>(minClusterActors, 1);
  minActorsToOffload = std::max<size_t>(minActorsToOffload, 1);
  minShardActors = std::max<size_t>(minShardActors, 1);

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
  config.repartitionIntervalTicks = ReadNumber<uint32_t>(
    j, "repartitionIntervalTicks", config.repartitionIntervalTicks);
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
    "minActorsToOffload={}, minClusterActors={}, minShardActors={}, "
    "separation={} chunks, interestManagement={} (fullRate={}u, maxSkip={}), "
    "adaptiveThrottling={}, budget={}us",
    workerThreads, minActorsToOffload, minClusterActors, minShardActors,
    clusterSeparationChunks, interestManagement ? "on" : "off",
    interestFullRateUnits, maxInterestSkipTicks,
    adaptiveThrottling ? "on" : "off", targetTickBudgetMicros);
}

}
