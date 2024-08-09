#include <catch2/catch_all.hpp>

#include "DummyMessageOutput.h"
#include "MovementValidation.h"
#include "NiPoint3.h"
#include <vector>

TEST_CASE("Returns true and sends nothing for normal movement",
          "[MovementValidation]")
{
  DummyMessageOutput messageOutput;

  bool res = MovementValidation::Validate(
    { 0, 0, 0 }, { 0, 0, 0 }, FormDesc::Tamriel(), { 1, 1, 1 },
    FormDesc::Tamriel(), messageOutput, { "Skyrim.esm" });
  REQUIRE(res);
  REQUIRE(messageOutput.messages.empty());
}

TEST_CASE("Returns false and sends teleport packet when moving too fast",
          "[MovementValidation]")
{
  DummyMessageOutput messageOutput;

  float maxLegalMove = 4096;

  bool res = MovementValidation::Validate(
    { 1, -1, 1 }, { 123, 111, 123 }, FormDesc::Tamriel(),
    NiPoint3{ 1, -1, 1 } + NiPoint3{ maxLegalMove, 0, 0 }, FormDesc::Tamriel(),
    messageOutput, { "Skyrim.esm" });
  REQUIRE(!res);
  REQUIRE(messageOutput.messages.size() == 1);
  REQUIRE(messageOutput.messages[0].j ==
          nlohmann::json{ { "type", "teleport" },
                          { "pos", { 1, -1, 1 } },
                          { "rot", { 123, 111, 123 } },
                          { "worldOrCell", 0x3c } });
}

TEST_CASE(
  "Returns false and sends teleport packet when moving between locations",
  "[MovementValidation]")
{
  DummyMessageOutput messageOutput;

  bool res = MovementValidation::Validate(
    { 1, -1, 1 }, { 123, 111, 123 }, FormDesc::Tamriel(), { 1, -1, 1 },
    FormDesc::FromString("ffffff:Skyrim.esm"), messageOutput,
    { "Skyrim.esm" });
  REQUIRE(!res);
  REQUIRE(messageOutput.messages.size() == 1);
  REQUIRE(messageOutput.messages[0].j ==
          nlohmann::json{ { "type", "teleport" },
                          { "pos", { 1, -1, 1 } },
                          { "rot", { 123, 111, 123 } },
                          { "worldOrCell", 0x3c } });
}
