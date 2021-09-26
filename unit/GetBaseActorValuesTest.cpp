#include "TestUtils.hpp"
#include <GroupUtils.h>
#include <Loader.h>
#include <catch2/catch.hpp>

extern espm::Loader l;

TEST_CASE("Testing values", "[GetBaseActorValues]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(
    0x0001B1DB); // Ri'saad( roving merchant from caravan. Khajiit)

  REQUIRE(form.rec->GetType() == "NPC_");
  auto npc = espm::Convert<espm::NPC_>(form.rec);

  espm::CompressedFieldsCache compressedFieldCache;
  auto raceId = npc->GetData(compressedFieldCache).race;
  auto raceForm = br.LookupById(raceId);
  REQUIRE(raceForm.rec->GetType() == "RACE");

  auto race = espm::Convert<espm::RACE>(raceForm.rec);
  auto data = race->GetData(compressedFieldCache);
  auto health = data.startingHealth;
  auto magicka = data.startingMagicka;
  auto stamina = data.startingStamina;
  auto healRate = data.healRegen;
  auto magickaRate = data.magickaRegen;
  auto staminaRate = data.staminaRegen;

  REQUIRE(health == 50.f);
  REQUIRE(magicka == 50.f);
  REQUIRE(stamina == 50.f);
  REQUIRE(healRate == 0.7f);
  REQUIRE(magickaRate == 3.f);
  REQUIRE(staminaRate == 5.f);

  REQUIRE(raceId == 0x00013745);
}
