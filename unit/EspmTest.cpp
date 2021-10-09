#include "TestUtils.hpp"
#include <GroupUtils.h>
#include <Loader.h>
#include <catch2/catch.hpp>

extern espm::Loader l;

// These tests depend on the most recent files shipped with Skyrim SE.
// See README.md in project root for details.

TEST_CASE("Hash check", "[espm]")
{
  const auto hashes = l.GetHashes();
  for (const auto& [filename, checksum] : hashes) {
    DYNAMIC_SECTION(filename << " checksum test")
    {
      REQUIRE(espm::GetCorrectHashcode(filename) == checksum);
    }
  }
}

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

TEST_CASE("Loads FormList", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0x21e81);
  REQUIRE(form.rec->GetType() == "FLST");

  auto data = reinterpret_cast<espm::FLST*>(form.rec)->GetData();
  REQUIRE(data.formIds == std::vector<uint32_t>({ 0x3eab9, 0x4e4bb }));
}

TEST_CASE("Loads refr with primitive", "[espm]")
{
  auto& br = l.GetBrowser();

  auto refr = br.LookupById(0xc07f0);
  REQUIRE(refr.rec);
  REQUIRE(refr.rec->GetType() == "REFR");

  auto data = reinterpret_cast<espm::REFR*>(refr.rec)->GetData();
  REQUIRE(abs(data.boundsDiv2[0] - 334.1504f) < 0.1);
  REQUIRE(abs(data.boundsDiv2[1] - 262.88865f) < 0.1);
  REQUIRE(abs(data.boundsDiv2[2] - 221.2002f) < 0.1);
}

TEST_CASE("Loads ConstructibleObject", "[espm]")
{
  enum
  {
    CraftingSmithingForge = 0x00088105
  };

  auto& br = l.GetBrowser();

  auto form = br.LookupById(0xdb89e);
  REQUIRE(form.rec->GetType() == "COBJ");

  auto data = reinterpret_cast<espm::COBJ*>(form.rec)->GetData();
  REQUIRE(data.benchKeywordId == CraftingSmithingForge);
  REQUIRE(data.outputObjectFormId == 0x1398a);
  REQUIRE(data.outputCount == 1);

  REQUIRE(data.inputObjects.size() == 3);
  REQUIRE(data.inputObjects[0].formId == 0x5ace4);
  REQUIRE(data.inputObjects[0].count == 1);
  REQUIRE(data.inputObjects[1].formId == 0x800e4);
  REQUIRE(data.inputObjects[1].count == 3);
  REQUIRE(data.inputObjects[2].formId == 0x5ace5);
  REQUIRE(data.inputObjects[2].count == 4);
}

TEST_CASE("Loads Furniture", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0xbbcf1);

  REQUIRE(form.rec->GetType() == "FURN");

  auto keywordIds = form.rec->GetKeywordIds();
  std::set<std::string> keywords;
  for (auto id : keywordIds) {
    keywords.insert(br.LookupById(id).rec->GetEditorId());
  }
  REQUIRE(keywords ==
          std::set<std::string>(
            { "CraftingSmithingForge", "CraftingSmithingSkyforge",
              "FurnitureForce3rdPerson", "FurnitureSpecial",
              "isBlacksmithForge", "RaceToScale", "WICraftingSmithing" }));
}

TEST_CASE("Loads Outfit", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0x10fe82);
  REQUIRE(form.rec);
  REQUIRE(form.rec->GetType() == espm::OTFT::type);

  auto outfit = espm::Convert<espm::OTFT>(form.rec);
  REQUIRE(outfit->GetData().count == 2);
  REQUIRE(outfit->GetData().formIds[0] == 0xabf4a);
  REQUIRE(outfit->GetData().formIds[1] == 0x10fe83);
}

TEST_CASE("Loads Npc", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0x7);
  REQUIRE(form.rec);
  REQUIRE(form.rec->GetType() == espm::NPC_::type);

  auto npc = espm::Convert<espm::NPC_>(form.rec);

  espm::CompressedFieldsCache compressedFieldsCache;
  REQUIRE(npc->GetData(compressedFieldsCache).defaultOutfitId == 0x1697c);
  REQUIRE(npc->GetData(compressedFieldsCache).sleepOutfitId == 0x0);
  REQUIRE(npc->GetData(compressedFieldsCache).objects.size() == 16);
}

TEST_CASE("Loads Weapon", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0x12eb7);
  REQUIRE(form.rec);
  REQUIRE(form.rec->GetType() == espm::WEAP::type);

  auto npc = espm::Convert<espm::WEAP>(form.rec);

  REQUIRE(npc->GetData().weapData);
  REQUIRE(npc->GetData().weapData->damage == 7);
  REQUIRE(npc->GetData().weapData->value == 25);
  REQUIRE(npc->GetData().weapData->weight == 9.f);
}

TEST_CASE("Loads NPC factions", "[espm]")
{
  enum
  {
    GuardWhiterunImperialGuardhouseSleep = 0x1000ef
  };

  auto& br = l.GetBrowser();

  auto form = br.LookupById(GuardWhiterunImperialGuardhouseSleep);
  REQUIRE(form.rec);
  REQUIRE(form.rec->GetType() == espm::NPC_::type);

  auto npc = espm::Convert<espm::NPC_>(form.rec);

  espm::CompressedFieldsCache compressedFieldsCache;
  REQUIRE(npc->GetData(compressedFieldsCache).factions.size() == 8);
  REQUIRE(npc->GetData(compressedFieldsCache).factions[0].formId == 0x28848);
  REQUIRE(npc->GetData(compressedFieldsCache).factions[0].rank == 72);
}

TEST_CASE("Loads NPC flags", "[espm]")
{
  enum
  {
    MQ304Kodlak = 0x9700e
  };

  auto& br = l.GetBrowser();

  auto form = br.LookupById(MQ304Kodlak);
  REQUIRE(form.rec);
  REQUIRE(form.rec->GetType() == espm::NPC_::type);

  auto npc = espm::Convert<espm::NPC_>(form.rec);

  espm::CompressedFieldsCache compressedFieldsCache;
  REQUIRE(npc->GetData(compressedFieldsCache).isEssential == true);
  REQUIRE(npc->GetData(compressedFieldsCache).isProtected == false);
}

TEST_CASE("Loads script names", "[espm]")
{
  auto& br = l.GetBrowser();
  auto res = br.LookupById(788464);

  REQUIRE(res.rec);
  espm::ScriptData scr;
  res.rec->GetScriptData(&scr);

  REQUIRE(scr.scripts.size() == 1);
  REQUIRE(scr.scripts[0].scriptName == "defaultsetStageTrigSCRIPT");
}

TEST_CASE("Correctly parses tree structure", "[espm]")
{
  auto& br = l.GetBrowser();

  auto form = br.LookupById(0xc07f0);
  REQUIRE(form.rec);

  std::vector<std::string> parentGroupTypeLabels;
  for (const auto groupPtr : br.GetParentGroupsEnsured(form.rec)) {
    parentGroupTypeLabels.emplace_back(
      ToString(groupPtr->GetGroupType()) + ":" +
      std::to_string(groupPtr->GetGroupLabelAsUint()));
  }
  REQUIRE(parentGroupTypeLabels ==
          std::vector<std::string>{
            "TOP:1145852503", // WRLD
            "WORLD_CHILDREN:107119",
            "CELL_CHILDREN:107120",
            "CELL_PERSISTENT_CHILDREN:107120",
          });

  const auto root = br.GetParentGroupsEnsured(form.rec)[0];
  REQUIRE(root);
  std::vector<uint32_t> records;
  espm::ForEachChildRecord(br, root,
                           [&records](const espm::RecordHeader* rec) {
                             records.emplace_back(rec->GetId());
                             return false;
                           });
  REQUIRE(records ==
          std::vector<uint32_t>{
            0x3c,    0x1691d, 0x16bb4, 0x16d71, 0x1a26f, 0x1b44a, 0x1cdd3,
            0x1cdd9, 0x1e49d, 0x1ee62, 0x1fae2, 0x204c7, 0x20bfe, 0x20dcb,
            0x21edb, 0x243de, 0x278dd, 0x29ab7, 0x2a9d8, 0x2b101, 0x2c965,
            0x2ee41, 0x34240, 0x35699, 0x37edf, 0x3a9d6, 0x419e1, 0x46033,
            0x4f838, 0x50015, 0x69857, 0x6ed38, 0x94b35, 0xc350d, 0xc97eb,
            0xd45f0, 0x104217 });
}

TEST_CASE("Parsing RACE", "[espm]")
{
  auto& br = l.GetBrowser();

  // Ri'saad is a Khajiit roving merchant from caravan.
  const uint32_t g_risaadFormId = 0x0001B1DB;

  auto form = br.LookupById(g_risaadFormId);

  REQUIRE(form.rec->GetType() == "NPC_");

  espm::CompressedFieldsCache compressedFieldsCache;

  auto npc = espm::Convert<espm::NPC_>(form.rec);
  uint32_t raceId = npc->GetData(compressedFieldsCache).race;
  auto raceInfo = l.GetBrowser().LookupById(raceId);

  REQUIRE(raceInfo.rec->GetType() == "RACE");
}
