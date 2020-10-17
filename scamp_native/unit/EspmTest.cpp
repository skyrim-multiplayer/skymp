#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include <espm.h>

#include <Loader.h>

extern espm::Loader l;

TEST_CASE("Loads refr from Update.esm", "[espm]")
{
  auto& br = l.GetBrowser();

  auto refr = br.LookupById(0x0100122a);
  REQUIRE(refr.rec);
  REQUIRE(refr.rec->GetType() == "REFR");
}

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
  REQUIRE(form.rec->GetType() == "TREE");

  auto data = reinterpret_cast<espm::TREE*>(form.rec)->GetData();
  REQUIRE(data.editorId == std::string("TreeFloraMountainFlower01Blue"));
  REQUIRE(data.resultItem == 0x77e1c);
  REQUIRE(data.useSound == 0x519d5);
}

TEST_CASE("Loads Flora", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0x7e8c9);
  REQUIRE(form.rec->GetType() == "FLOR");

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

TEST_CASE("Loads script-related subrecords for SovngardeWatcherStatue2",
          "[espm]")
{
  auto& br = l.GetBrowser();
  auto form = br.LookupById(0x105d05);
  REQUIRE(form.rec->GetType() == "ACTI");

  auto data = reinterpret_cast<espm::ACTI*>(form.rec)->GetData();
  REQUIRE(data.scriptData.version == 5);
  REQUIRE(data.scriptData.objFormat == 2);
  REQUIRE(data.scriptData.scripts.size() == 1);
  REQUIRE(data.scriptData.scripts.front().scriptName ==
          "sovngardestatuescript");
  REQUIRE(data.scriptData.scripts.front().properties.size() == 1);
  REQUIRE(data.scriptData.scripts.front().properties.begin()->propertyName ==
          "MQ305");
  REQUIRE(data.scriptData.scripts.front().properties.begin()->propertyType ==
          espm::PropertyType::Object);
  REQUIRE(data.scriptData.scripts.front().properties.begin()->status == 1);
  REQUIRE(data.scriptData.scripts.front().properties.begin()->value.formId ==
          0x46ef2);
}

TEST_CASE("Loads script-related subrecords for BearTrap01", "[espm]")
{
  auto& br = l.GetBrowser();
  auto form = br.LookupById(0x7144d);
  REQUIRE(form.rec->GetType() == "ACTI");

  auto data = reinterpret_cast<espm::ACTI*>(form.rec)->GetData();
  REQUIRE(data.scriptData.version == 5);
  REQUIRE(data.scriptData.objFormat == 2);

  REQUIRE(data.scriptData.scripts.size() == 2);

  REQUIRE(data.scriptData.scripts[0].scriptName == "TrapBear");
  REQUIRE(data.scriptData.scripts[0].properties ==
          std::set<espm::Property>{
            espm::Property::Object("LightFoot", 0x5820c),
            espm::Property::Object("LightFootTriggerPercent", 0x67194),
            espm::Property::Int("LvlDamage1", 20),
            espm::Property::Int("LvlDamage2", 25),
            espm::Property::Int("LvlDamage3", 30),
            espm::Property::Int("LvlDamage4", 35),
            espm::Property::Int("LvlDamage5", 40),
            espm::Property::Int("LvlDamage6", 45),
            espm::Property::Int("LvlThreshold1", 6),
            espm::Property::Int("LvlThreshold2", 10),
            espm::Property::Int("LvlThreshold3", 14),
            espm::Property::Int("LvlThreshold4", 18),
            espm::Property::Int("LvlThreshold5", 22) });

  REQUIRE(data.scriptData.scripts[1].scriptName == "TrapHitBase");
  REQUIRE(data.scriptData.scripts[1].properties ==
          std::set<espm::Property>{
            espm::Property::Bool("canDisease", true),
            espm::Property::Object("GhostAbility", 0x5030b),
            espm::Property::Bool("hitOnlyOnce", true),
            espm::Property::Bool("rumble", true),
            espm::Property::Float("rumbleAmount", 0.2f),
            espm::Property::Float("rumbleDuration", 0.5f),
            espm::Property::Float("staggerAmount", 0.5f),
            espm::Property::Bool("trapCausesStagger", true),
            espm::Property::Object("TrapDiseaseAtaxia", 0x10a24a),
            espm::Property::Object("TrapDiseaseBoneBreakFever", 0x10a24c),
            espm::Property::Object("TrapDiseaseBrainRot", 0x10a24d),
            espm::Property::Object("TrapDiseaseRattles", 0x10a24e),
            espm::Property::Object("TrapDiseaseRockjoint", 0x10a24f),
            espm::Property::Object("TrapDiseaseWitbane", 0x10a250) });
}