#include <catch2/catch.hpp>

#include "DummyMessageOutput.h"
#include "DummyWorldObject.h"
#include "MovementValidation.h"
#include "NiPoint3.h"
#include <vector>

TEST_CASE("Returns true and sends nothing for normal movement",
          "[MovementValidation]")
{
  DummyWorldObject obj({ 0, 0, 0 }, { 0, 0, 0 }, 0x3c);
  DummyMessageOutput messageOutput;

  bool res =
    MovementValidation::Validate(obj, { 1, 1, 1 }, 0x3c, messageOutput);
  REQUIRE(res);
}

TEST_CASE("Returns false when moving too fast",
          "[MovementValidation]")
{
  DummyWorldObject obj({ 1, -1, 1 }, { 123, 111, 123 }, 0x3c);
  DummyMessageOutput messageOutput;

  float maxLegalMove = 4096;

  bool res = MovementValidation::Validate(
    obj, obj.GetPos() + NiPoint3{ maxLegalMove, 0, 0 }, 0x3c, messageOutput);
  REQUIRE(!res);
}


TEST_CASE(
  "Returns false when moving between locations",
  "[MovementValidation]")
{
  DummyWorldObject obj({ 1, -1, 1 }, { 123, 111, 123 }, 0x3c);
  DummyMessageOutput messageOutput;

  bool res = MovementValidation::Validate(
    obj, obj.GetPos(), 0x00ffffff, messageOutput);
  REQUIRE(!res);
}