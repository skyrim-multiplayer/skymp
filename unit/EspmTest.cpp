#include "TestUtils.hpp"
#include "libespm/GroupUtils.h"
#include "libespm/Loader.h"
#include <catch2/catch_all.hpp>

extern espm::Loader l;

// These tests depend on the files shipped with Skyrim SE (pre-AE update).
// See README.md in project root for details.

TEST_CASE("Hash check", "[espm]")
{
  const auto hashes = l.GetFilesInfo();
  for (const auto& [filename, info] : hashes) {
    DYNAMIC_SECTION(filename << " checksum and size test")
    {
      REQUIRE(espm::GetCorrectHashcode(filename) == info.crc32);
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

TEST_CASE("Loads Container", "[espm]")
{
  auto& br = l.GetBrowser();
  espm::CompressedFieldsCache cache;

  auto barrel = br.LookupById(0x10cd5b);
  REQUIRE(barrel.rec->GetType() == "CONT");

  auto barrelData = reinterpret_cast<espm::CONT*>(barrel.rec)->GetData(cache);

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
  espm::CompressedFieldsCache cache;

  auto form = br.LookupById(0xbcf3d);
  REQUIRE(form.rec->GetType() == "TREE");

  auto data = reinterpret_cast<espm::TREE*>(form.rec)->GetData(cache);
  REQUIRE(data.editorId == std::string("TreeFloraMountainFlower01Blue"));
  REQUIRE(data.resultItem == 0x77e1c);
  REQUIRE(data.useSound == 0x519d5);
}

TEST_CASE("Loads Flora", "[espm]")
{
  auto& br = l.GetBrowser();
  espm::CompressedFieldsCache cache;

  auto form = br.LookupById(0x7e8c9);
  REQUIRE(form.rec->GetType() == "FLOR");

  auto data = reinterpret_cast<espm::FLOR*>(form.rec)->GetData(cache);
  REQUIRE(data.editorId == std::string("BirdsNest"));
  REQUIRE(data.resultItem == 0x7e8c8);
  REQUIRE(data.useSound == 0x100f88);
}

TEST_CASE("Loads LeveledItem", "[espm]")
{
  auto& br = l.GetBrowser();
  espm::CompressedFieldsCache cache;

  auto form = br.LookupById(0x10e992);
  REQUIRE(form.rec->GetType() == "LVLI");

  auto data = reinterpret_cast<espm::LVLI*>(form.rec)->GetData(cache);
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
  espm::CompressedFieldsCache cache;
  auto form = br.LookupById(0x105d05);
  REQUIRE(form.rec->GetType() == "ACTI");

  auto data = reinterpret_cast<espm::ACTI*>(form.rec)->GetData(cache);
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
  espm::CompressedFieldsCache cache;
  auto form = br.LookupById(0x7144d);
  REQUIRE(form.rec->GetType() == "ACTI");

  auto data = reinterpret_cast<espm::ACTI*>(form.rec)->GetData(cache);
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
  espm::CompressedFieldsCache cache;

  auto form = br.LookupById(0x21e81);
  REQUIRE(form.rec->GetType() == "FLST");

  auto data = reinterpret_cast<espm::FLST*>(form.rec)->GetData(cache);
  REQUIRE(data.formIds == std::vector<uint32_t>({ 0x3eab9, 0x4e4bb }));
}

TEST_CASE("Loads refr with primitive", "[espm]")
{
  auto& br = l.GetBrowser();
  espm::CompressedFieldsCache cache;

  auto refr = br.LookupById(0xc07f0);
  REQUIRE(refr.rec);
  REQUIRE(refr.rec->GetType() == "REFR");

  auto data = reinterpret_cast<espm::REFR*>(refr.rec)->GetData(cache);
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
  espm::CompressedFieldsCache cache;

  auto form = br.LookupById(0xdb89e);
  REQUIRE(form.rec->GetType() == "COBJ");

  auto data = reinterpret_cast<espm::COBJ*>(form.rec)->GetData(cache);
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
  espm::CompressedFieldsCache cache;

  auto form = br.LookupById(0xbbcf1);

  REQUIRE(form.rec->GetType() == "FURN");

  auto keywordIds = form.rec->GetKeywordIds(cache);
  std::set<std::string> keywords;
  for (auto id : keywordIds) {
    keywords.insert(br.LookupById(id).rec->GetEditorId(cache));
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
  espm::CompressedFieldsCache cache;

  auto form = br.LookupById(0x10fe82);
  REQUIRE(form.rec);
  REQUIRE(form.rec->GetType() == espm::OTFT::kType);

  auto outfit = espm::Convert<espm::OTFT>(form.rec);
  REQUIRE(outfit->GetData(cache).count == 2);
  REQUIRE(outfit->GetData(cache).formIds[0] == 0xabf4a);
  REQUIRE(outfit->GetData(cache).formIds[1] == 0x10fe83);
}

TEST_CASE("Loads Npc", "[espm]")
{
  auto& br = l.GetBrowser();
  espm::CompressedFieldsCache cache;

  auto form = br.LookupById(0x7);
  REQUIRE(form.rec);
  REQUIRE(form.rec->GetType() == espm::NPC_::kType);

  auto npc = espm::Convert<espm::NPC_>(form.rec);

  espm::CompressedFieldsCache compressedFieldsCache;
  REQUIRE(npc->GetData(compressedFieldsCache).defaultOutfitId == 0x1697c);
  REQUIRE(npc->GetData(compressedFieldsCache).sleepOutfitId == 0x0);
  REQUIRE(npc->GetData(compressedFieldsCache).objects.size() == 16);
}

TEST_CASE("Loads Weapon", "[espm]")
{
  auto& br = l.GetBrowser();
  espm::CompressedFieldsCache cache;

  auto form = br.LookupById(0x12eb7);
  REQUIRE(form.rec);
  REQUIRE(form.rec->GetType() == espm::WEAP::kType);

  auto npc = espm::Convert<espm::WEAP>(form.rec);

  REQUIRE(npc->GetData(cache).weapData);
  REQUIRE(npc->GetData(cache).weapData->damage == 7);
  REQUIRE(npc->GetData(cache).weapData->value == 25);
  REQUIRE(npc->GetData(cache).weapData->weight == 9.f);
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
  REQUIRE(form.rec->GetType() == espm::NPC_::kType);

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
  REQUIRE(form.rec->GetType() == espm::NPC_::kType);

  auto npc = espm::Convert<espm::NPC_>(form.rec);

  espm::CompressedFieldsCache compressedFieldsCache;
  REQUIRE(npc->GetData(compressedFieldsCache).isEssential == true);
  REQUIRE(npc->GetData(compressedFieldsCache).isProtected == false);
}

TEST_CASE("Loads script names", "[espm]")
{
  auto& br = l.GetBrowser();
  espm::CompressedFieldsCache cache;
  auto res = br.LookupById(788464);

  REQUIRE(res.rec);
  espm::ScriptData scr;
  res.rec->GetScriptData(&scr, cache);

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
  const uint32_t kRisaadFormId = 0x0001B1DB;

  auto form = br.LookupById(kRisaadFormId);

  REQUIRE(form.rec->GetType() == "NPC_");

  espm::CompressedFieldsCache compressedFieldsCache;

  auto npc = espm::Convert<espm::NPC_>(form.rec);
  uint32_t raceId = npc->GetData(compressedFieldsCache).race;
  auto raceInfo = l.GetBrowser().LookupById(raceId);

  REQUIRE(raceInfo.rec->GetType() == "RACE");
}

namespace {
class MyEspmProvider
{
public:
  espm::Loader& GetEspm() { return l; }

  espm::CompressedFieldsCache& GetEspmCache()
  {
    return compressedFieldsCache;
  };

private:
  espm::CompressedFieldsCache compressedFieldsCache;
};
}

TEST_CASE("espm::GetData wrapper is able to get record data", "[espm]")
{
  MyEspmProvider provider;

  constexpr uint32_t kArgonianRace = 0x00013740;
  constexpr uint32_t kIronSword = 0x00012eb7;

  REQUIRE_THROWS_WITH(espm::GetData<espm::RACE>(
                        0xDEADBEEF, static_cast<MyEspmProvider*>(nullptr)),
                      Catch::Matchers::ContainsSubstring(
                        "Unable to find record without EspmProvider"));

  REQUIRE_THROWS_WITH(
    espm::GetData<espm::RACE>(0xDEADBEEF, &provider),
    Catch::Matchers::ContainsSubstring("Record 0xdeadbeef doesn't exist"));

  REQUIRE_THROWS_WITH(espm::GetData<espm::RACE>(kIronSword, &provider),
                      Catch::Matchers::ContainsSubstring(
                        "Expected record 0x12eb7 to be RACE, but found WEAP"));

  // Verify that data wasn't default constructed
  auto data = espm::GetData<espm::RACE>(kArgonianRace, &provider);
  REQUIRE(abs(data.healRegen - 0.7f) < std::numeric_limits<float>::epsilon());
}

TEST_CASE("GMST parsing", "[espm]")
{
  MyEspmProvider provider;
  REQUIRE(
    espm::GetData<espm::GMST>(espm::GMST::kFMaxArmorRating, &provider).value ==
    80);
}

TEST_CASE("ARMO parsing", "[espm]")
{
  MyEspmProvider provider;
  {
    // Iron Gauntlets
    auto data = espm::GetData<espm::ARMO>(0x12e46, &provider);
    REQUIRE(data.baseRatingX100 == 1000);
    REQUIRE(data.baseValue == 25);
    REQUIRE(data.weight == 5);
  }
  {
    // Iron Boots
    auto data = espm::GetData<espm::ARMO>(0x12e4b, &provider);
    REQUIRE(data.baseRatingX100 == 1000);
    REQUIRE(data.baseValue == 25);
    REQUIRE(data.weight == 6);
  }
  {
    // Iron Boots
    auto data = espm::GetData<espm::ARMO>(0x12e4d, &provider);
    REQUIRE(data.baseRatingX100 == 1500);
    REQUIRE(data.baseValue == 60);
    REQUIRE(data.weight == 5);
  }
  {
    // Hide Boots
    auto data = espm::GetData<espm::ARMO>(0x13910, &provider);
    REQUIRE(data.baseRatingX100 == 500);
    REQUIRE(data.baseValue == 10);
    REQUIRE(data.weight == 1);
  }
  {
    // Hide Armor
    auto data = espm::GetData<espm::ARMO>(0x13911, &provider);
    REQUIRE(data.baseRatingX100 == 2000);
    REQUIRE(data.baseValue == 50);
    REQUIRE(data.weight == 5);
    REQUIRE(data.enchantmentFormId == 0);
  }
  {
    // Steel Plate Boots with fire resist
    auto data = espm::GetData<espm::ARMO>(0x1B401, &provider);
    REQUIRE(data.baseRatingX100 == 1400);
    REQUIRE(data.baseValue == 125);
    REQUIRE(data.weight == 9);
    REQUIRE(data.enchantmentFormId == 0xAD484); // fire resist 03
  }
}

TEST_CASE("ENCH and effects parsing", "[espm]")
{
  MyEspmProvider provider;
  {
    // EnchArmorResistFire03
    auto data = espm::GetData<espm::ENCH>(0xAD484, &provider);
    REQUIRE(data.effects.size() == 1);
    auto& effect = data.effects[0];
    REQUIRE(effect.effectId == 0x48C8B);
    REQUIRE(effect.areaOfEffect == 0);
    REQUIRE(effect.duration == 0);
    REQUIRE(effect.magnitude == 40);
  }
  {
    // EnchWeaponNightingaleBow04
    auto data = espm::GetData<espm::ENCH>(0xF6521, &provider);
    REQUIRE(data.effects.size() == 3);
    auto& effects = data.effects;
    REQUIRE(effects[0].effectId == 0x4605B); // EnchFrostDamageFFContact
    REQUIRE(effects[0].areaOfEffect == 0);
    REQUIRE(effects[0].duration == 0);
    REQUIRE(effects[0].magnitude == 25);

    REQUIRE(effects[1].effectId == 0x4605C); // EnchShockDamageFFContact
    REQUIRE(effects[1].areaOfEffect == 0);
    REQUIRE(effects[1].duration == 1);
    REQUIRE(effects[1].magnitude == 12);

    REQUIRE(effects[2].effectId == 0xB72A0); // FrostSlowFFContact
    REQUIRE(effects[2].areaOfEffect == 0);
    REQUIRE(effects[2].duration == 3);
    REQUIRE(effects[2].magnitude == 50);
  }
}

TEST_CASE("MGEF parsing", "[espm]")
{
  MyEspmProvider provider;
  auto data = espm::GetData<espm::MGEF>(0x51B15, &provider);
  REQUIRE(data.data.primaryAV == espm::ActorValue::DamageResist);
}
