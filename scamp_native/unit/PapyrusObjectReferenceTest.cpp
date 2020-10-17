#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "MpObjectReference.h"
#include "PapyrusObjectReference.h"

TEST_CASE("GetItemCount", "[Papyrus][ObjectReference]")
{
  PartOne p;
  auto refr = std::make_unique<MpObjectReference>(
    LocationalData(), FormCallbacks::DoNothing(), 0, "CONT");
  p.worldState.AddForm(std::move(refr), 0xff000000);

  auto self =
    p.worldState.GetFormAt<MpObjectReference>(0xff000000).ToVarValue();

  REQUIRE(PapyrusObjectReference::GetItemCount(self, {}) == VarValue(0));
}