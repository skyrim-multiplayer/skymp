#include "TestUtils.hpp"

// PartOne.h only forward-declares these, so a consumer that touches the
// dispatcher or reads the metrics needs the real definitions.
#include "parallel/OffloadDispatcher.h"
#include "parallel/ParallelConfig.h"
#include "parallel/ParallelMetrics.h"

// The same movement scenarios as PartOne_MovementTest.cpp, but with the
// multi-core area offload switched on.
//
// This is the test that matters for the framework: it drives the real
// PartOne, the real packet parser and the real send target, and asserts the
// offloaded path produces the same observable messages as the inline one.
//
// One deliberate difference is exercised here rather than hidden: with the
// offload enabled a movement relay is emitted during PartOne::Tick instead of
// during packet ingest, so each scenario ticks before asserting. Everything
// else - counts, contents, recipients, resulting actor transform - must
// match the inline expectations exactly.

namespace {

MpParallel::ParallelConfig EnabledConfig()
{
  MpParallel::ParallelConfig config;
  config.enabled = true;
  config.workerThreads = 4;
  // Offload even tiny batches, so the tests actually take the parallel path
  // rather than silently falling back to the inline one.
  config.minActorsToOffload = 1;
  config.minClusterActors = 1;
  config.minShardActors = 1;
  // Deterministic: no relay may be deferred by throttling.
  config.adaptiveThrottling = false;
  config.Normalize();
  return config;
}

}

TEST_CASE("Parallel: UpdateMovement relays to a neighbour", "[PartOne]")
{
  PartOne partOne;
  partOne.ConfigureParallelism(EnabledConfig());
  REQUIRE(partOne.GetOffloadDispatcher().IsEnabled());

  DoConnect(partOne, 0);

  DoMessage(partOne, 0, jMovement);
  partOne.Tick();
  REQUIRE(partOne.Messages().size() == 0); // No actor - no movement

  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xff000ABC);
  partOne.Messages().clear();

  DoMessage(partOne, 0, jMovement);
  // Nothing has gone out yet: the relay is built on a worker and flushed by
  // the join.
  REQUIRE(partOne.Messages().size() == 0);

  partOne.Tick();
  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages().at(0).j == jMovement);

  // The transform must have been applied by the join, exactly as the inline
  // path applies it inside the packet handler.
  auto* actor = dynamic_cast<MpActor*>(
    partOne.worldState.LookupFormById(0xff000ABC).get());
  REQUIRE(actor->GetPos() == NiPoint3{ 1, -1, 1 });
  REQUIRE(actor->GetAngle() == NiPoint3{ 0, 0, 179 });
}

TEST_CASE("Parallel: two players see each other move", "[PartOne]")
{
  PartOne partOne;
  partOne.ConfigureParallelism(EnabledConfig());

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xff000ABC);

  DoConnect(partOne, 1);
  partOne.CreateActor(0xff00ABCD, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(1, 0xff00ABCD);

  partOne.Messages().clear();
  DoMessage(partOne, 0, jMovement);
  partOne.Tick();

  // Both the sender and the neighbour receive it, same as inline.
  REQUIRE(partOne.Messages().size() == 2);
  REQUIRE(partOne.Messages().at(0).j == jMovement);
  REQUIRE(partOne.Messages().at(1).j == jMovement);
}

TEST_CASE("Parallel: disconnected neighbour stops receiving", "[PartOne]")
{
  PartOne partOne;
  partOne.ConfigureParallelism(EnabledConfig());

  for (int i = 0; i < 2; ++i) {
    DoConnect(partOne, i);
    partOne.CreateActor(i + 0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
    partOne.SetUserActor(i, i + 0xff000ABC);
    auto m = jMovement;
    m["idx"] = i;
    DoMessage(partOne, i, m);
    partOne.Tick();
  }

  DoDisconnect(partOne, 1);

  partOne.Messages().clear();
  DoMessage(partOne, 0, jMovement);
  partOne.Tick();
  REQUIRE(partOne.Messages().size() == 1);
}

TEST_CASE("Parallel: many actors, only connected users are relayed to",
          "[PartOne]")
{
  // Mirrors the "Hypothesis" case: half the actors have no user attached.
  PartOne partOne;
  partOne.ConfigureParallelism(EnabledConfig());

  constexpr uint32_t n = 20;
  static_assert(n <= MAX_PLAYERS - 1);

  for (uint32_t i = 0; i < n; ++i) {
    partOne.CreateActor(i + 0xff000000, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
    if (i % 2 == 0) {
      continue;
    }
    DoConnect(partOne, i + 1);
    partOne.SetUserActor(i + 1, i + 0xff000000);
    DoUpdateMovement(partOne, i + 0xff000000, i + 1);
    partOne.Tick();
  }

  DoConnect(partOne, 0);
  partOne.CreateActor(0xffffffff, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xffffffff);
  partOne.Messages().clear();

  DoUpdateMovement(partOne, 0xffffffff, 0);
  partOne.Tick();
  REQUIRE(partOne.Messages().size() == 11); // Me and 10 other users
}

TEST_CASE("Parallel: a crowd is sharded and still fully relayed", "[PartOne]")
{
  // Everyone in one place, which is the case sharding exists for. Every
  // player must receive one relay from every player, including themselves.
  PartOne partOne;
  partOne.ConfigureParallelism(EnabledConfig());

  constexpr uint32_t n = 12;
  for (uint32_t i = 0; i < n; ++i) {
    DoConnect(partOne, i);
    partOne.CreateActor(0xff001000 + i, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
    partOne.SetUserActor(i, 0xff001000 + i);
  }

  partOne.Messages().clear();

  for (uint32_t i = 0; i < n; ++i) {
    auto m = jMovement;
    m["idx"] = dynamic_cast<MpActor*>(
                 partOne.worldState.LookupFormById(0xff001000 + i).get())
                 ->GetIdx();
    DoMessage(partOne, i, m);
  }
  partOne.Tick();

  const auto& metrics = partOne.GetParallelMetrics();
  REQUIRE(metrics.lastActorCount == n);
  // All in one chunk, so one cluster; sharding is what spreads it.
  REQUIRE(metrics.lastClusterCount == 1);
  REQUIRE(metrics.lastWorkUnitCount > 1);

  // n senders x n recipients, none dropped.
  REQUIRE(partOne.Messages().size() == n * n);
  REQUIRE(metrics.lastRelayEdgesThrottled == 0);
}

TEST_CASE("Parallel: repeated ticks stay consistent", "[PartOne]")
{
  PartOne partOne;
  partOne.ConfigureParallelism(EnabledConfig());

  constexpr uint32_t n = 6;
  for (uint32_t i = 0; i < n; ++i) {
    DoConnect(partOne, i);
    partOne.CreateActor(0xff002000 + i, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
    partOne.SetUserActor(i, 0xff002000 + i);
  }

  for (int round = 0; round < 25; ++round) {
    partOne.Messages().clear();
    for (uint32_t i = 0; i < n; ++i) {
      auto m = jMovement;
      m["idx"] = dynamic_cast<MpActor*>(
                   partOne.worldState.LookupFormById(0xff002000 + i).get())
                   ->GetIdx();
      DoMessage(partOne, i, m);
    }
    partOne.Tick();
    REQUIRE(partOne.Messages().size() == n * n);
  }
}
