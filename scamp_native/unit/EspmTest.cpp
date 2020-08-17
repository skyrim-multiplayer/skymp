#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include <espm.h>

#include <Loader.h>

extern espm::Loader l;

TEST_CASE("Loads Iron Sword", "[espm]")
{
  auto& br = l.GetBrowser();

  auto ironSword = br.LookupById(0x12eb7);
  REQUIRE(ironSword.rec->GetType() == "WEAP");
}

TEST_CASE("Loads Container", "[espm]")
{
  auto& br = l.GetBrowser();

  auto barrel = br.LookupById(0x10cd5b);
  REQUIRE(barrel.rec->GetType() == "CONT");

  auto barrelData = reinterpret_cast<espm::CONT*>(barrel.rec)->GetData();

  REQUIRE(barrelData.editorId == std::string("BarrelFish01"));
  REQUIRE(barrelData.objects.size() == 2);
  REQUIRE(barrelData.objects[0].count == 1);
  REQUIRE(barrelData.objects[0].formId == 0x10d964);
  REQUIRE(barrelData.objects[1].count == 1);
  REQUIRE(barrelData.objects[1].formId == 0x10cd5e);
}

TEST_CASE("Loads Tree", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0xbcf3d);
  REQUIRE(form.rec->GetType() == espm::TREE::type);

  auto data = reinterpret_cast<espm::TREE*>(form.rec)->GetData();
  REQUIRE(data.editorId == std::string("TreeFloraMountainFlower01Blue"));
  REQUIRE(data.resultItem == 0x77e1c);
  REQUIRE(data.useSound == 0x519d5);
}

TEST_CASE("Loads Flora", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0x7e8c9);
  REQUIRE(form.rec->GetType() == espm::FLOR::type);

  auto data = reinterpret_cast<espm::FLOR*>(form.rec)->GetData();
  REQUIRE(data.editorId == std::string("BirdsNest"));
  REQUIRE(data.resultItem == 0x7e8c8);
  REQUIRE(data.useSound == 0x100f88);
}

TEST_CASE("Loads LeveledItem", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0x10e992);
  REQUIRE(form.rec->GetType() == "LVLI");

  auto data = reinterpret_cast<espm::LVLI*>(form.rec)->GetData();
  REQUIRE(data.editorId == std::string("lItemGems10"));
  REQUIRE(data.leveledItemFlags == (espm::LVLI::AllLevels | espm::LVLI::Each));
  REQUIRE((int)data.numEntries == 48);
  REQUIRE(data.entries[0].count == 1);
  REQUIRE(data.entries[0].level == 1);
  REQUIRE(data.entries[0].formId == 0x63b45);
  REQUIRE(data.entries[47].count == 1);
  REQUIRE(data.entries[47].level == 46);
  REQUIRE(data.entries[47].formId == 0x6851f);
  REQUIRE(data.chanceNone == 90);
}