#include "parallel/InterestManager.h"
#include "parallel/OffloadDispatcher.h"
#include <algorithm>
#include <array>
#include <catch2/catch_all.hpp>
#include <map>
#include <memory>
#include <vector>

using namespace MpParallel;

namespace {

struct RecordedRelay
{
  Networking::UserId userId = Networking::InvalidUserId;
  std::vector<uint8_t> bytes;
  bool reliable = false;

  bool operator<(const RecordedRelay& rhs) const
  {
    if (userId != rhs.userId) {
      return userId < rhs.userId;
    }
    if (bytes != rhs.bytes) {
      return bytes < rhs.bytes;
    }
    return reliable < rhs.reliable;
  }

  bool operator==(const RecordedRelay& rhs) const
  {
    return userId == rhs.userId && bytes == rhs.bytes &&
      reliable == rhs.reliable;
  }
};

class RecordingSink : public IOffloadSink
{
public:
  void ApplyMovement(const ActorSnapshot& actor) override
  {
    applied.push_back(actor.formId);
    appliedPos[actor.formId] = { actor.proposedPos[0], actor.proposedPos[1],
                                 actor.proposedPos[2] };
  }

  void SendCorrection(const ActorSnapshot& actor) override
  {
    corrected.push_back(actor.formId);
  }

  void SendRelay(Networking::UserId userId, const uint8_t* data, size_t length,
                 bool reliable) override
  {
    RecordedRelay relay;
    relay.userId = userId;
    relay.bytes.assign(data, data + length);
    relay.reliable = reliable;
    relays.push_back(relay);
  }

  std::vector<uint32_t> applied;
  std::vector<uint32_t> corrected;
  std::vector<RecordedRelay> relays;
  std::map<uint32_t, std::array<float, 3>> appliedPos;
};

ParallelConfig MakeConfig(size_t workerThreads, size_t minActorsToOffload)
{
  ParallelConfig config;
  config.enabled = true;
  config.workerThreads = workerThreads;
  config.minActorsToOffload = minActorsToOffload;
  config.minClusterActors = 1;
  // Both rate-reduction mechanisms off, so these tests observe the raw relay
  // fan-out. Interest management gets its own dedicated cases below.
  config.adaptiveThrottling = false;
  config.interestManagement = false;
  config.Normalize();
  return config;
}

// Builds a submission whose proposed position is a small, always-valid step
// away from where the actor currently is.
MovementSubmission MakeSubmission(uint32_t formId, uint32_t idx,
                                  Networking::UserId ownerUserId, float x,
                                  float y, const std::vector<uint8_t>& packet)
{
  MovementSubmission submission;
  submission.formId = formId;
  submission.idx = idx;
  submission.ownerUserId = ownerUserId;

  submission.currentPos[0] = x;
  submission.currentPos[1] = y;
  submission.currentWorldOrCell = 0x3c;

  submission.proposedPos[0] = x + 10.f;
  submission.proposedPos[1] = y + 10.f;
  submission.proposedWorldOrCell = 0x3c;

  submission.isStanding = true;
  submission.packetData = packet.data();
  submission.packetLength = packet.size();
  
  return submission;
}

RelayTarget MakeTarget(Networking::UserId userId, uint32_t formId, float x,
                       float y)
{
  RelayTarget target;
  target.userId = userId;
  target.listenerFormId = formId;
  target.worldOrCell = 0x3c;
  target.chunkX = static_cast<int16_t>(x / 4096.f);
  target.chunkY = static_cast<int16_t>(y / 4096.f);
  target.pos[0] = x;
  target.pos[1] = y;
  return target;
}

}

TEST_CASE("A disabled dispatcher declines everything", "[ParallelOffload]")
{
  ParallelConfig config;
  config.Normalize();
  OffloadDispatcher dispatcher(config);

  REQUIRE_FALSE(dispatcher.IsEnabled());

  const std::vector<uint8_t> packet{ 1, 2, 3 };
  std::vector<RelayTarget> targets{ MakeTarget(7, 0xff000002, 0.f, 0.f) };
  /* SetPotentialTargets handled */
  REQUIRE_FALSE(dispatcher.SubmitMovement(
    MakeSubmission(0xff000001, 1, 5, 0.f, 0.f, packet)));
  REQUIRE(dispatcher.GetPendingCount() == 0);

  RecordingSink sink;
  dispatcher.ExecuteTick(sink);
  REQUIRE(sink.relays.empty());
  REQUIRE(sink.applied.empty());
}

TEST_CASE("A submission without a packet is declined", "[ParallelOffload]")
{
  OffloadDispatcher dispatcher(MakeConfig(0, 1));

  MovementSubmission submission;
  submission.formId = 0xff000001;
  submission.packetData = nullptr;
  submission.packetLength = 0;

  REQUIRE_FALSE(dispatcher.SubmitMovement(submission));
  REQUIRE(dispatcher.GetPendingCount() == 0);
}

TEST_CASE("Accepted movement is applied and relayed", "[ParallelOffload]")
{
  OffloadDispatcher dispatcher(MakeConfig(0, 1));

  const std::vector<uint8_t> packet{ 0x10, 0x20, 0x30 };
  const std::vector<RelayTarget> targets{
    MakeTarget(2, 0xff000002, 100.f, 100.f),
    MakeTarget(3, 0xff000003, 200.f, 200.f),
  };

  REQUIRE(dispatcher.SubmitMovement(
    MakeSubmission(0xff000001, 1, 1, 0.f, 0.f, packet)));
  REQUIRE(dispatcher.GetPendingCount() == 1);

  // Recipients now come from the per-tick snapshot of active players rather
  // than from the submission, so the dispatcher has to be given the list.
  dispatcher.SetPotentialTargets(std::vector<RelayTarget>(targets));

  RecordingSink sink;
  dispatcher.ExecuteTick(sink);

  REQUIRE(sink.applied == std::vector<uint32_t>{ 0xff000001 });
  REQUIRE(sink.corrected.empty());
  REQUIRE(sink.relays.size() == 2);
  REQUIRE(sink.relays[0].bytes == packet);
  REQUIRE(sink.relays[1].bytes == packet);
  REQUIRE_FALSE(sink.relays[0].reliable);

  // The pending set is consumed by the tick.
  REQUIRE(dispatcher.GetPendingCount() == 0);
}

TEST_CASE("Validation mirrors the inline rules", "[ParallelOffload]")
{
  ActorSnapshot actor;
  actor.currentWorldOrCell = 0x3c;
  actor.proposedWorldOrCell = 0x3c;

  SECTION("A small step is accepted")
  {
    actor.proposedPos[0] = 100.f;
    REQUIRE(InterestManager::ValidateMovement(actor));
  }

  SECTION("A cell change is rejected")
  {
    actor.proposedWorldOrCell = 0x1a26f;
    REQUIRE_FALSE(InterestManager::ValidateMovement(actor));
  }

  SECTION("A step of a full cell or more is rejected")
  {
    actor.proposedPos[0] = 4096.f;
    REQUIRE_FALSE(InterestManager::ValidateMovement(actor));
  }

  SECTION("Just under a full cell is accepted")
  {
    actor.proposedPos[0] = 4095.f;
    REQUIRE(InterestManager::ValidateMovement(actor));
  }

  SECTION("The teleport flag always rejects")
  {
    actor.teleportFlag = true;
    actor.proposedPos[0] = 1.f;
    REQUIRE_FALSE(InterestManager::ValidateMovement(actor));
  }

  SECTION("Distance is measured in three dimensions")
  {
    actor.proposedPos[0] = 3000.f;
    actor.proposedPos[1] = 3000.f;
    REQUIRE_FALSE(InterestManager::ValidateMovement(actor));
  }
}

TEST_CASE("A rejected update corrects its owner but still relays",
          "[ParallelOffload]")
{
  OffloadDispatcher dispatcher(MakeConfig(0, 1));

  const std::vector<uint8_t> packet{ 0xaa };
  const std::vector<RelayTarget> targets{
    MakeTarget(2, 0xff000002, 10.f, 10.f)
  };

  MovementSubmission submission =
    MakeSubmission(0xff000001, 1, 1, 0.f, 0.f, packet);
  // Well beyond one cell: must fail validation.
  submission.proposedPos[0] = 100000.f;

  REQUIRE(dispatcher.SubmitMovement(submission));
  dispatcher.SetPotentialTargets(std::vector<RelayTarget>(targets));

  RecordingSink sink;
  dispatcher.ExecuteTick(sink);

  REQUIRE(sink.applied.empty());
  REQUIRE(sink.corrected == std::vector<uint32_t>{ 0xff000001 });
  // The inline path relays before validating, so the neighbour still sees it.
  REQUIRE(sink.relays.size() == 1);
}

TEST_CASE("A hosted NPC is never sent a correction", "[ParallelOffload]")
{
  OffloadDispatcher dispatcher(MakeConfig(0, 1));

  const std::vector<uint8_t> packet{ 0xbb };
  std::vector<RelayTarget> targets;

  MovementSubmission submission = MakeSubmission(
    0xff000009, 9, Networking::InvalidUserId, 0.f, 0.f, packet);
  submission.proposedPos[0] = 100000.f;

  REQUIRE(dispatcher.SubmitMovement(submission));

  RecordingSink sink;
  dispatcher.ExecuteTick(sink);

  REQUIRE(sink.applied.empty());
  REQUIRE(sink.corrected.empty());
}

TEST_CASE("Parallel and inline runs produce the same work",
          "[ParallelOffload]")
{
  // Same inputs, one run forced inline and one spread over four workers. The
  // set of effects has to match; only the order may differ.
  const std::vector<uint8_t> packet{ 0x01, 0x02 };

  auto buildAndRun = [&](size_t workers, size_t threshold) {
    OffloadDispatcher dispatcher(MakeConfig(workers, threshold));

    // One flat list of active players for the whole tick. Recipients are
    // derived from it by spatial filtering, so every user id must be distinct
    // or the same player would appear in several chunks.
    std::vector<RelayTarget> allTargets;
    allTargets.reserve(64 * 5);

    for (uint32_t i = 0; i < 64; ++i) {
      // Ten separate crowds, far enough apart to become distinct clusters.
      const float baseX = static_cast<float>((i % 10) * 200000);
      const float baseY = static_cast<float>((i / 10) * 100);

      for (uint32_t t = 0; t < 5; ++t) {
        allTargets.push_back(
          MakeTarget(static_cast<Networking::UserId>(100 + i * 5 + t),
                     0xff001000 + i * 5 + t, baseX + 50.f, baseY + 50.f));
      }

      REQUIRE(dispatcher.SubmitMovement(MakeSubmission(
        0xff000001 + i, i, static_cast<Networking::UserId>(i), baseX, baseY,
        packet)));
    }

    dispatcher.SetPotentialTargets(std::move(allTargets));

    auto sink = std::make_unique<RecordingSink>();
    dispatcher.ExecuteTick(*sink);
    return sink;
  };

  auto inlineSink = buildAndRun(0, 100000);
  auto parallelSink = buildAndRun(4, 1);

  // The exact fan-out depends on how many players share a chunk, which is not
  // what this test is about. What matters is that the work happened and that
  // both configurations produced identical effects.
  REQUIRE(inlineSink->relays.size() > 0);
  REQUIRE(parallelSink->relays.size() == inlineSink->relays.size());

  std::sort(inlineSink->relays.begin(), inlineSink->relays.end());
  std::sort(parallelSink->relays.begin(), parallelSink->relays.end());
  REQUIRE(inlineSink->relays == parallelSink->relays);

  std::sort(inlineSink->applied.begin(), inlineSink->applied.end());
  std::sort(parallelSink->applied.begin(), parallelSink->applied.end());
  REQUIRE(inlineSink->applied == parallelSink->applied);
  REQUIRE(inlineSink->applied.size() == 64);
}

TEST_CASE("Join order is stable across repeated identical ticks",
          "[ParallelOffload]")
{
  const std::vector<uint8_t> packet{ 0x77 };

  auto run = [&] {
    OffloadDispatcher dispatcher(MakeConfig(4, 1));
    std::vector<std::vector<RelayTarget>> targetStorage;
    targetStorage.reserve(40);

    for (uint32_t i = 0; i < 40; ++i) {
      const float baseX = static_cast<float>((i % 8) * 500000);
      targetStorage.emplace_back();
      targetStorage.back().push_back(
        MakeTarget(static_cast<Networking::UserId>(i), 0xff002000 + i,
                   baseX, 0.f));
      REQUIRE(dispatcher.SubmitMovement(MakeSubmission(
        0xff000001 + i, i, static_cast<Networking::UserId>(i), baseX, 0.f,
        packet)));
    }

    RecordingSink sink;
    dispatcher.ExecuteTick(sink);
    return sink.applied;
  };

  const std::vector<uint32_t> first = run();
  for (int attempt = 0; attempt < 5; ++attempt) {
    REQUIRE(run() == first);
  }
}

TEST_CASE("Throttling only fires under pressure and spares close players",
          "[ParallelOffload]")
{
  ParallelConfig config;
  config.enabled = true;
  config.adaptiveThrottling = true;
  config.throttleDistanceUnits = 4096.f;
  config.maxThrottleSkipTicks = 4;
  // Isolate the pressure mechanism from the always-on distance one.
  config.interestManagement = false;
  config.Normalize();

  SECTION("No pressure means no throttling at any distance")
  {
    REQUIRE(InterestManager::ComputeSkipFactor(0.f, 0, config) == 1);
    REQUIRE(InterestManager::ComputeSkipFactor(1e12f, 0, config) == 1);
  }

  SECTION("Under pressure, nearby players are still never throttled")
  {
    REQUIRE(InterestManager::ComputeSkipFactor(0.f, 3, config) == 1);
    // Exactly at the threshold distance.
    REQUIRE(InterestManager::ComputeSkipFactor(4096.f * 4096.f, 3, config) ==
            1);
  }

  SECTION("Distant players are spaced out, more so the further they are")
  {
    const float sqrNear = 4097.f * 4097.f;
    const float sqrFar = 20000.f * 20000.f;
    const uint32_t nearFactor =
      InterestManager::ComputeSkipFactor(sqrNear, 1, config);
    const uint32_t farFactor =
      InterestManager::ComputeSkipFactor(sqrFar, 1, config);
    REQUIRE(nearFactor > 1);
    REQUIRE(farFactor >= nearFactor);
    REQUIRE(farFactor <= config.maxThrottleSkipTicks);
  }

  SECTION("Disabling the feature pins the factor at one")
  {
    ParallelConfig off = config;
    off.adaptiveThrottling = false;
    REQUIRE(InterestManager::ComputeSkipFactor(1e12f, 4, off) == 1);
  }
}

TEST_CASE("Interest management reduces distant update rate on an idle server",
          "[ParallelOffload]")
{
  ParallelConfig config;
  config.enabled = true;
  // No pressure, no adaptive throttling: this must work purely on distance.
  config.adaptiveThrottling = false;
  config.interestManagement = true;
  config.interestFullRateUnits = 2048.f;
  config.maxInterestSkipTicks = 4;
  config.Normalize();

  const float sqrFull = 2048.f * 2048.f;

  SECTION("Close recipients keep every single update")
  {
    REQUIRE(InterestManager::ComputeSkipFactor(0.f, 0, config) == 1);
    REQUIRE(InterestManager::ComputeSkipFactor(sqrFull * 0.5f, 0, config) == 1);
    // Exactly on the boundary still counts as close.
    REQUIRE(InterestManager::ComputeSkipFactor(sqrFull, 0, config) == 1);
  }

  SECTION("Rate drops in steps as distance grows")
  {
    const uint32_t a = InterestManager::ComputeSkipFactor(sqrFull * 2.f, 0, config);
    const uint32_t b = InterestManager::ComputeSkipFactor(sqrFull * 8.f, 0, config);
    const uint32_t c =
      InterestManager::ComputeSkipFactor(sqrFull * 64.f, 0, config);
    REQUIRE(a == 2);
    REQUIRE(b == 3);
    REQUIRE(c == 4);
    REQUIRE(c <= config.maxInterestSkipTicks);
  }

  SECTION("The cap is respected")
  {
    ParallelConfig capped = config;
    capped.maxInterestSkipTicks = 2;
    capped.Normalize();
    REQUIRE(InterestManager::ComputeSkipFactor(1e12f, 0, capped) == 2);
  }

  SECTION("Turning it off restores full rate everywhere")
  {
    ParallelConfig off = config;
    off.interestManagement = false;
    off.Normalize();
    REQUIRE(InterestManager::ComputeSkipFactor(1e12f, 0, off) == 1);
  }

  SECTION("Pressure and distance stack by taking the stronger of the two")
  {
    ParallelConfig both = config;
    both.adaptiveThrottling = true;
    both.throttleDistanceUnits = 4096.f;
    both.maxThrottleSkipTicks = 4;
    both.Normalize();

    // Close enough that pressure spares it, but interest management still
    // applies its own floor rather than being overridden back to 1.
    const uint32_t nearUnderPressure =
      InterestManager::ComputeSkipFactor(sqrFull * 2.f, 3, both);
    REQUIRE(nearUnderPressure >= 2);
    REQUIRE(nearUnderPressure <= 4);
  }
}

TEST_CASE("Every distant edge still transmits within its window",
          "[ParallelOffload]")
{
  // Reducing the rate must never silence a pair permanently.
  for (uint32_t skip = 2; skip <= 4; ++skip) {
    for (uint32_t sender = 0xff000001; sender < 0xff000008; ++sender) {
      for (Networking::UserId user = 0; user < 6; ++user) {
        int sent = 0;
        for (uint64_t tick = 0; tick < skip; ++tick) {
          if (InterestManager::ShouldRelayThisTick(tick, sender, user, skip)) {
            ++sent;
          }
        }
        REQUIRE(sent == 1);
      }
    }
  }
}

TEST_CASE("A throttled edge still transmits within its skip window",
          "[ParallelOffload]")
{
  // Every pair must get a turn: a skip factor of N means one tick in N, not
  // silence.
  constexpr uint32_t kSkipFactor = 3;
  for (uint32_t senderFormId = 0xff000001; senderFormId < 0xff000010;
       ++senderFormId) {
    for (Networking::UserId userId = 0; userId < 8; ++userId) {
      int sent = 0;
      for (uint64_t tick = 0; tick < kSkipFactor; ++tick) {
        if (InterestManager::ShouldRelayThisTick(tick, senderFormId, userId,
                                                 kSkipFactor)) {
          ++sent;
        }
      }
      REQUIRE(sent == 1);
    }
  }
}

TEST_CASE("Skip factor of one always transmits", "[ParallelOffload]")
{
  for (uint64_t tick = 0; tick < 16; ++tick) {
    REQUIRE(InterestManager::ShouldRelayThisTick(tick, 0xff000001, 3, 1));
    REQUIRE(InterestManager::ShouldRelayThisTick(tick, 0xff000001, 3, 0));
  }
}

TEST_CASE("Metrics track relays and cluster counts", "[ParallelOffload]")
{
  OffloadDispatcher dispatcher(MakeConfig(2, 1));

  const std::vector<uint8_t> packet{ 0x55 };
  std::vector<RelayTarget> allTargets;

  for (uint32_t i = 0; i < 8; ++i) {
    // Far enough apart that each player is alone in its own chunk, so every
    // sender relays to exactly one recipient: itself.
    const float baseX = static_cast<float>(i * 500000);
    allTargets.push_back(MakeTarget(static_cast<Networking::UserId>(i),
                                    0xff003000 + i, baseX, 0.f));
    REQUIRE(dispatcher.SubmitMovement(MakeSubmission(
      0xff000001 + i, i, static_cast<Networking::UserId>(i), baseX, 0.f,
      packet)));
  }
  dispatcher.SetPotentialTargets(std::move(allTargets));

  RecordingSink sink;
  dispatcher.ExecuteTick(sink);

  const ParallelMetrics& metrics = dispatcher.GetMetrics();
  REQUIRE(metrics.lastActorCount == 8);
  REQUIRE(metrics.lastClusterCount == 8);
  REQUIRE(metrics.lastLargestClusterSize == 1);
  // Each cluster holds one actor, which is below minShardActors, so no
  // cluster is split and units == clusters.
  REQUIRE(metrics.lastWorkUnitCount == 8);
  REQUIRE(metrics.lastRelayEdgesEmitted == 8);
  REQUIRE(metrics.lastRelayEdgesThrottled == 0);
  REQUIRE(metrics.totalTicks == 1);
  REQUIRE(metrics.totalFailedTasks == 0);
}

TEST_CASE("One crowded area is split across cores", "[ParallelOffload]")
{
  // The case the whole design exists for: nearly everyone in one place. If a
  // cluster were the unit of work this would be a single task and the extra
  // cores would sit idle.
  ParallelConfig config = MakeConfig(4, 1);
  config.minShardActors = 4;
  config.Normalize();
  OffloadDispatcher dispatcher(config);

  const std::vector<uint8_t> packet{ 0x42 };
  std::vector<RelayTarget> allTargets;
  allTargets.reserve(60);

  for (uint32_t i = 0; i < 60; ++i) {
    // All inside one chunk, so the partitioner must produce exactly one
    // cluster.
    const float x = static_cast<float>(i % 10) * 10.f;
    const float y = static_cast<float>(i / 10) * 10.f;

    allTargets.push_back(
      MakeTarget(static_cast<Networking::UserId>(i), 0xff004000 + i, x, y));

    REQUIRE(dispatcher.SubmitMovement(MakeSubmission(
      0xff000001 + i, i, static_cast<Networking::UserId>(i), x, y, packet)));
  }
  dispatcher.SetPotentialTargets(std::move(allTargets));

  RecordingSink sink;
  dispatcher.ExecuteTick(sink);

  const ParallelMetrics& metrics = dispatcher.GetMetrics();
  REQUIRE(metrics.lastClusterCount == 1);
  REQUIRE(metrics.lastLargestClusterSize == 60);
  // The point of the exercise: one cluster, many schedulable units.
  REQUIRE(metrics.lastWorkUnitCount > 1);
  REQUIRE(metrics.lastPooledUnitCount > 1);

  // Sharding must not lose or duplicate anyone. All 60 share a chunk, so
  // every sender relays to all 60 recipients: the full N^2 fan-out, intact.
  REQUIRE(sink.applied.size() == 60);
  REQUIRE(sink.relays.size() == 60 * 60);

  std::vector<uint32_t> expected;
  for (uint32_t i = 0; i < 60; ++i) {
    expected.push_back(0xff000001 + i);
  }
  // Join order is still ascending submission order despite the split.
  REQUIRE(sink.applied == expected);
}

TEST_CASE("Shard count does not change the outcome", "[ParallelOffload]")
{
  // One crowd, processed as one unit and as many. Same effects either way.
  const std::vector<uint8_t> packet{ 0x11, 0x22 };

  auto run = [&](size_t maxShards) {
    ParallelConfig config = MakeConfig(4, 1);
    config.minShardActors = 2;
    config.maxShardsPerCluster = maxShards;
    config.Normalize();
    OffloadDispatcher dispatcher(config);

    std::vector<RelayTarget> allTargets;
    allTargets.reserve(50 * 3);
    for (uint32_t i = 0; i < 50; ++i) {
      const float x = static_cast<float>(i) * 20.f;
      for (uint32_t t = 0; t < 3; ++t) {
        // Distinct user ids: one entry per active player in the snapshot.
        allTargets.push_back(
          MakeTarget(static_cast<Networking::UserId>(200 + i * 3 + t),
                     0xff005000 + i * 3 + t, x, 0.f));
      }
      REQUIRE(dispatcher.SubmitMovement(MakeSubmission(
        0xff000001 + i, i, static_cast<Networking::UserId>(i), x, 0.f, packet)));
    }
    dispatcher.SetPotentialTargets(std::move(allTargets));

    auto sink = std::make_unique<RecordingSink>();
    dispatcher.ExecuteTick(*sink);
    return sink;
  };

  auto oneShard = run(1);
  auto manyShards = run(16);

  REQUIRE(manyShards->applied == oneShard->applied);
  REQUIRE(manyShards->corrected == oneShard->corrected);
  // Relays keep their exact order too, not merely their contents: units are
  // joined in cluster order then ascending member order.
  REQUIRE(manyShards->relays == oneShard->relays);
  // 50 senders spanning x=0..980, all inside one chunk, against 150 snapshot
  // entries in that chunk.
  REQUIRE(oneShard->relays.size() == 50 * 150);
}

TEST_CASE("Reconfigure drops pending work and rebuilds the pool",
          "[ParallelOffload]")
{
  OffloadDispatcher dispatcher(MakeConfig(2, 1));

  const std::vector<uint8_t> packet{ 0x99 };
  std::vector<RelayTarget> targets{ MakeTarget(1, 0xff000002, 0.f, 0.f) };
  /* SetPotentialTargets handled */
  REQUIRE(dispatcher.SubmitMovement(
    MakeSubmission(0xff000001, 1, 1, 0.f, 0.f, packet)));
  REQUIRE(dispatcher.GetPendingCount() == 1);

  ParallelConfig disabled;
  disabled.Normalize();
  dispatcher.Reconfigure(disabled);

  REQUIRE(dispatcher.GetPendingCount() == 0);
  REQUIRE_FALSE(dispatcher.IsEnabled());

  RecordingSink sink;
  dispatcher.ExecuteTick(sink);
  REQUIRE(sink.relays.empty());
}

TEST_CASE("An empty tick is cheap and harmless", "[ParallelOffload]")
{
  OffloadDispatcher dispatcher(MakeConfig(2, 1));
  RecordingSink sink;

  for (int i = 0; i < 10; ++i) {
    dispatcher.ExecuteTick(sink);
  }

  REQUIRE(sink.relays.empty());
  REQUIRE(dispatcher.GetMetrics().totalTicks == 10);
}
