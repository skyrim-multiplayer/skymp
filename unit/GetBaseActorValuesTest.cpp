#include "TestUtils.hpp"
#include <GroupUtils.h>
#include <Loader.h>
#include <catch2/catch.hpp>

extern espm::Loader l;

TEST_CASE("Testing values", "[GetBaseActorValues]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0x7);

  REQUIRE(form.rec->GetType() == "NPC_");
  auto npc = espm::Convert<espm::NPC_>(form.rec);

  espm::CompressedFieldsCache compressedFieldCache;
  auto raceId = npc->GetData(compressedFieldCache).race; // 0x00013746

  auto another_form = br.LookupById(raceId);
  REQUIRE(another_form.rec->GetType() == "RACE");


  REQUIRE(raceId == 0x00013740);
}
