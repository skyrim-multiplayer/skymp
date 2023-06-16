#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

TEST_CASE("User changes", "[ActorsMap]")
{
  ActorsMap map;
  auto actor = std::make_shared<MpActor>(LocationalData(), FormCallbacks());
  map.Set(0, actor.get());
  map.Set(1, actor.get());

  REQUIRE(map.Find(static_cast<Networking::UserId>(0)) == nullptr);
  REQUIRE(map.Find(static_cast<Networking::UserId>(1)) == actor.get());
  REQUIRE(map.Find(actor.get()) == 1);
}

TEST_CASE("Actor changes", "[ActorsMap]")
{
  ActorsMap map;
  auto actorA = std::make_shared<MpActor>(LocationalData(), FormCallbacks());
  auto actorB = std::make_shared<MpActor>(LocationalData(), FormCallbacks());
  map.Set(0, actorA.get());
  map.Set(0, actorB.get());

  REQUIRE(map.Find(static_cast<Networking::UserId>(0)) == actorB.get());
  REQUIRE(map.Find(actorB.get()) == 0);
  REQUIRE(map.Find(actorA.get()) == Networking::InvalidUserId);
}
