#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "EspmGameObject.h"
#include "MpObjectReference.h"
#include "PapyrusObjectReference.h"

extern espm::Loader l;

namespace {
auto CreateMpObjectReference(PartOne& partOne, uint32_t id)
{
  auto refr = std::make_unique<MpObjectReference>(
    LocationalData(), FormCallbacks::DoNothing(), 0, "CONT");
  partOne.worldState.AddForm(std::move(refr), id);
}
}

TEST_CASE("GetItemCount/AddItem", "[Papyrus][ObjectReference]")
{
  PartOne p;

  CreateMpObjectReference(p, 0xff000000);

  EspmGameObject ironSword(l.GetBrowser().LookupById(0x12eb7));
  auto& refr = p.worldState.GetFormAt<MpObjectReference>(0xff000000);

  auto item = VarValue(&ironSword);

  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(0));

  refr.AddItem(0x12eb7, 10);
  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(10));
}

TEST_CASE("RemoveItem", "[Papyrus][ObjectReference]")
{
  PartOne p;

  CreateMpObjectReference(p, 0xff000000);
  CreateMpObjectReference(p, 0xff000001);

  EspmGameObject ironSword(l.GetBrowser().LookupById(0x12eb7));
  auto& refr = p.worldState.GetFormAt<MpObjectReference>(0xff000000);
  auto& refr2 = p.worldState.GetFormAt<MpObjectReference>(0xff000001);

  auto item = VarValue(&ironSword);

  refr.AddItem(0x12eb7, 10);
  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(10));

  PapyrusObjectReference().RemoveItem(refr.ToVarValue(),
                                      { item, VarValue(5), VarValue::None() });

  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(5));

  PapyrusObjectReference().RemoveItem(
    refr.ToVarValue(), { item, VarValue(-1), refr2.ToVarValue() });

  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(0));

  REQUIRE(PapyrusObjectReference().GetItemCount(refr2.ToVarValue(),
                                                { item }) == VarValue(5));
}
