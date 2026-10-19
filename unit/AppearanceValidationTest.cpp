#include "AppearanceValidation.h"
#include "TestUtils.hpp"

using Catch::Matchers::ContainsSubstring;

PartOne& GetPartOne();

static Appearance MakeKhajiitAppearance()
{
  Appearance a;
  a.name = "Khajiit Test";
  a.raceId = 0x00013745; // KhajiitRace (playable)
  a.isFemale = false;
  a.weight = 50.f;
  return a;
}

TEST_CASE("AppearanceValidation - valid playable race passes", "[espm]")
{
  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;

  auto appearance = MakeKhajiitAppearance();
  auto error = AppearanceValidation::Validate(worldState, appearance);

  REQUIRE(error.empty());
}

TEST_CASE("AppearanceValidation - non-playable race is rejected", "[espm]")
{
  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;

  auto appearance = MakeKhajiitAppearance();
  appearance.raceId = 0x000E7713; // non-playable NPC race

  auto error = AppearanceValidation::Validate(worldState, appearance);

  REQUIRE(!error.empty());
  REQUIRE_THAT(error, ContainsSubstring("not a playable race"));
}

TEST_CASE("AppearanceValidation - non-existent raceId is rejected", "[espm]")
{
  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;

  auto appearance = MakeKhajiitAppearance();
  appearance.raceId = 0xDEADBEEF;

  auto error = AppearanceValidation::Validate(worldState, appearance);

  REQUIRE(!error.empty());
  REQUIRE_THAT(error, ContainsSubstring("not found in espm"));
}

TEST_CASE("AppearanceValidation - empty name is rejected")
{
  PartOne partOne; // no espm
  auto& worldState = partOne.worldState;

  Appearance appearance;
  appearance.name = "";
  appearance.raceId = 0x00013745;

  auto error = AppearanceValidation::Validate(worldState, appearance);

  REQUIRE(!error.empty());
  REQUIRE_THAT(error, ContainsSubstring("name must not be empty"));
}

TEST_CASE("AppearanceValidation - name exceeding max length is rejected")
{
  PartOne partOne; // no espm
  auto& worldState = partOne.worldState;

  Appearance appearance;
  appearance.name = std::string(65, 'x');
  appearance.raceId = 0x00013745;

  auto error = AppearanceValidation::Validate(worldState, appearance);

  REQUIRE(!error.empty());
  REQUIRE_THAT(error, ContainsSubstring("exceeds maximum length"));
}

TEST_CASE("AppearanceValidation - name with control characters is rejected")
{
  PartOne partOne; // no espm
  auto& worldState = partOne.worldState;

  Appearance appearance;
  appearance.name = "Valid\x01Name";
  appearance.raceId = 0x00013745;

  auto error = AppearanceValidation::Validate(worldState, appearance);

  REQUIRE(!error.empty());
  REQUIRE_THAT(error, ContainsSubstring("invalid control characters"));
}

TEST_CASE("AppearanceValidation - no espm loaded skips race check")
{
  PartOne partOne; // no espm
  auto& worldState = partOne.worldState;

  Appearance appearance;
  appearance.name = "ValidName";
  appearance.raceId = 0xDEADBEEF; // would fail if espm were loaded

  auto error = AppearanceValidation::Validate(worldState, appearance);

  REQUIRE(error.empty());
}
