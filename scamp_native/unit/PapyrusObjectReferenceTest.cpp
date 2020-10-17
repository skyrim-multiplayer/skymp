#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "EspmGameObject.h"
#include "MpObjectReference.h"
#include "PapyrusObjectReference.h"

extern espm::Loader l;

TEST_CASE("GetItemCount", "[Papyrus][ObjectReference]")
{
  PartOne p;
  {
    auto refr = std::make_unique<MpObjectReference>(
      LocationalData(), FormCallbacks::DoNothing(), 0, "CONT");
    p.worldState.AddForm(std::move(refr), 0xff000000);
  }
  EspmGameObject ironSword(l.GetBrowser().LookupById(0x12eb7));
  auto& refr = p.worldState.GetFormAt<MpObjectReference>(0xff000000);

  auto self = refr.ToVarValue();
  auto item = VarValue(&ironSword);

  REQUIRE(PapyrusObjectReference::GetItemCount(self, { item }) == VarValue(0));

  refr.AddItem(0x12eb7, 10);
  REQUIRE(PapyrusObjectReference::GetItemCount(self, { item }) ==
          VarValue(10));
}