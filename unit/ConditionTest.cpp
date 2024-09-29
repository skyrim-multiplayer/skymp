#include "Condition.h"
#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

TEST_CASE("ItemCountCondition evaluates correctly", "[ItemCountCondition]")
{
  MpActor actor(LocationalData(), FormCallbacks::DoNothing());

  ItemCountCondition condition(0x1, 5, espm::CTDA::Operator::EqualTo,
                               espm::CTDA::Flags::ANDORDEFAULT);

  REQUIRE(condition.Evaluate(&actor) == false);

  actor.AddItem(0x1, 4);

  REQUIRE(condition.Evaluate(&actor) == false);

  actor.AddItem(0x1, 1);

  REQUIRE(condition.Evaluate(&actor) == true);
}

TEST_CASE("RaceCondition evaluates correctly", "[RaceCondition]")
{
  MpActor actor(LocationalData(), FormCallbacks::DoNothing());
  Appearance appearance;
  appearance.raceId = 0x123;
  actor.SetAppearance(&appearance);

  RaceCondition condition(0x21, 1, espm::CTDA::Operator::EqualTo,
                          espm::CTDA::Flags::ANDORDEFAULT);

  REQUIRE(condition.Evaluate(&actor) == false);

  condition = RaceCondition(0x123, 1, espm::CTDA::Operator::EqualTo,
                            espm::CTDA::Flags::ANDORDEFAULT);

  REQUIRE(condition.Evaluate(&actor) == true);
}
