#include "PieScript.h"

#include "FormDesc.h"
#include "MpActor.h"
#include "SpSnippet.h"
#include "WorldState.h"
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

std::mt19937 g_rng{ std::random_device{}() };

int GenerateRandomNumber(int leftBound, int rightBound)
{
  if (leftBound <= rightBound) {
    std::uniform_int_distribution<> distr(leftBound, rightBound);
    return distr(g_rng);
  } else {
    throw std::runtime_error(fmt::format(
      "GenerateRandomNumber() cannot generate number in range: ({}, {})",
      leftBound, rightBound));
  }
}

void PieScript::AddDLCItems(const std::vector<std::string>& espmFiles,
                            const std::vector<std::string>& items,
                            LootboxItemType type, Tier tier)
{
  for (const auto& item : items) {
    FormDesc formDesc = FormDesc::FromString(item);
    uint32_t id = formDesc.ToFormId(espmFiles);

    lootTable[type][tier].push_back(id);
  }
}
PieScript::PieScript(const std::vector<std::string>& espmFiles)
{
  lootTable = {
    { LootboxItemType::Weapon,
      {
        {
          Tier::Tier1,
          {
            0x0001397E,
            0x00013790,
            0x00013981,
            0x0002C66F,
            0x0001CB64,
            0x00012EB7,
            0x00013980,
            0x000CADE9,
            0x0002C672,
            0x0001359D,
            0x00013982,
            0x000CC829,
            0x000236A5,
          },
        },
        { Tier::Tier2,
          { 0x00013986, 0x00013983, 0x0001398A, 0x00013997, 0x00013998,
            0x000139A1, 0x0001399C, 0x0001398E, 0x0001398B, 0x00013992,
            0x00013989, 0x00013984, 0x00013996, 0x00013993, 0x0001399A,
            0x0001399F, 0x000139A0, 0x00013991, 0x0001398C, 0x0010AA19,
            0x00013987, 0x00013988, 0x00013999, 0x00013994, 0x0001399E,
            0x0001399B, 0x000139A2, 0x0001398F, 0x00013990, 0x0010C6FB,
            0x0002E6D1, 0x000302CD } },
        { Tier::Tier3,
          { 0x000BE25E, 0x000139A6, 0x000139A9, 0x000139A8, 0x000F82FA,
            0x000139AA, 0x000139A3, 0x000139A9, 0x0007A91A, 0x0003AEB9,
            0x000139A7, 0x000139A4, 0x00f71dd } },
        { Tier::Tier4,
          { 0x000139AF, 0x000139B0, 0x000CDEC9, 0x000139B2, 0x000139B2,
            0x000139AE, 0x000139B1, 0x000139AC, 0x000F8313, 0x0007A917 } },
        { Tier::Tier5,
          { 0x000139B6, 0x000139B3, 0x000139BA, 0x000240D2, 0x000233E3,
            0x000139B9, 0x000139B4, 0x0009CCDC, 0x0004A38F, 0x0002ACD2,
            0x0001C4E6, 0x000956B5, 0x0004E4EE, 0x0006A13C, 0x000139B7,
            0x000139B8 } },
      } },
    { LootboxItemType::Armor,
      {
        { Tier::Tier1,
          { 0x00013913, 0x00013922, 0x00012E4D, 0x0001B3A1, 0x0006F39E,
            0x000D8D52, 0x00056A9E, 0x00012E46, 0x00013912, 0x000D8D55,
            0x00012EB6, 0x000209A6, 0x0006ff38, 0x0005B69F, 0x0005b6a1,
            0x0006FF45, 0x000209A5, 0x000209A5, 0x000C5D12, 0x00013911,
            0x0003619E, 0x00012E49, 0x0001B3A3, 0x0001B3A4, 0x0010594D,
            0x000D8D50, 0x00018388, 0x00013921, 0x00013921, 0x0001394B,
            0x0001be1a, 0x000f1229, 0x000209a5, 0x0006c1d9, 0x0004223C,
            0x0001BE1B, 0x0001BE1B, 0x000BACD7, 0x0006FF37, 0x00013910,
            0x00013920, 0x00012E4B, 0x0001B39F, 0x0006F398, 0x000D8D4E,
            0x00056a9D, 0x0006F39B, 0x0001B3A0, 0x00013914, 0x0006c1d9,
            0x0006ff43, 0x0006C1D8, 0x0003452E, 0x000D191F, 0x0003452F,
            0x000C36E8 } },
        { Tier::Tier2,
          { 0x00013954, 0x000F6F24, 0x0001395E, 0x00013959, 0x0001391D,
            0x0001394F, 0x0004C3CB, 0x000d2842, 0x000D3AC5, 0x000B83CF,
            0x00013958, 0x00013955, 0x00013950, 0x000CEE80, 0x000CEE82,
            0x0010E2CE, 0x000F8715, 0x000F8713, 0x00013946, 0x0001392A,
            0x0001394F, 0x00013953, 0x000D3AC4, 0x000D3AC3, 0x000d2844,
            0x000B83CB, 0x0001394D, 0x0001392A, 0x00013957, 0x00013952,
            0x00013952, 0x00013951, 0x0001395B, 0x00013956, 0x0001391A,
            0x0001394C, 0x000B83CD, 0x000D2845, 0x000D3AC2, 0x000D2843,
            0x0001395D, 0x0001394E, 0x0001391E, 0x00086991, 0x00086993,
            0x0005C06C } },
        { Tier::Tier3,
          { 0x0004C3D0, 0x0004B28F, 0x000cf8b2, 0x0004F912, 0x000E84C4,
            0x0008698C, 0x0006230B, 0x0007BC19, 0x000E0DD0, 0x000E0DD2,
            0x0007BC1A, 0x00065BB3, 0x0010C698, 0x00065BBF, 0x000E84C6,
            0x000EAD49, 0x000CF8B1, 0x000CEE7E, 0x000cf8b3, 0x0004B28B,
            0x000CAE15, 0x000CEE7C, 0x0004B288, 0x0004B28D, 0x000CF8B0,
            0x0001393B, 0x000CEE76, 0x00062311, 0x00065BAC, 0x0005DB7E,
            0x0007BC15, 0x0001393C, 0x0001393A, 0x00013939, 0x00013938 } },
        { Tier::Tier4,
          { 0x00013963, 0x00013969, 0x00013967, 0x00013964, 0x00061CCA,
            0x000CEE72, 0x000CEE6E, 0x00015516, 0x000FC5BF, 0x00013962,
            0x00013966, 0x00013961, 0x00013960, 0x00013965, 0x00013941,
            0x000CEE70, 0x000CEE74 } },
        { Tier::Tier5,
          { 0x0001396D, 0x0001396C, 0x00052794, 0x00088952, 0x000C7CBB,
            0x00045F96, 0x0001396E, 0x0001396B, 0x0001396A, 0x0002AC61 } },
      } },
    { LootboxItemType::Consumable,
      {
        { Tier::Tier1,
          {
            0x0004B0BA, 0x000D8E3F, 0x0005076E, 0x0001D4EC, 0x08557B54,
            0x000DB5D2, 0x08557B55, 0x000C886C, 0x0005ACDB, 0x00071CF3,
            0x00034CDD, 0x0005ACE2, 0x0005ACE1, 0x0401CD7C, 0x0005ACDD,
            0x0005ACE0, 0x0402B04E, 0x0003AD53, 0x0003AD57,
          } },
        { Tier::Tier2,
          { 0x0006AC4A, 0x0001B3BD, 0x00064B2E, 0x00064B2F, 0x00023D77,
            0x08557B55 } },
        {
          Tier::Tier3,
          { 0x0003ADA4, 0x0005ACDC, 0x0402B06B, 0x0003AD5B },
        },
        { Tier::Tier4, { 0x0003EADE, 0x0005ACDF } },
        { Tier::Tier5,
          {
            0x0003EAE3,
          } },
      } }
  };

  starterKitsMap = {
    {
      StarterKitType::ChefKit,
      { 0x0001BCA7, 0x000261C1, 0x0001BC82, 0x0001F25B, 0x000D1921,
        0x084D2B03 },
    },
    { StarterKitType::LumberjackKit,
      {
        0x000209AA,
        0x000261C0,
        0x000261BD,
        0x0002F2F4,
        0x000261C1,
      } },
    {
      StarterKitType::MinerKit,
      { 0x000330B3, 0x00080697, 0x00080699, 0x000E3C16, 0x0010E039 },
    },
    { StarterKitType::PrisonerKit,
      { 0x000646AB, 0x0008F19A, 0x0003ca00, 0x000426C8, 0x0010E039,
        0x0851F7E4 } },
    { StarterKitType::PatronKit,
      { 0x0008895A, 0x000B145B, 0x0001C1FE, 0x00088956, 0x00088958 } },
  };

  std::vector<std::string> weaponTier3 = {
    "00D098:Dawnguard.esm",   "01CDB1:Dragonborn.esm", "01CDAD:Dragonborn.esm",
    "00DD55:Dawnguard.esm",   "01CDAF:Dragonborn.esm", "01CDB0:Dragonborn.esm",
    "00084E:HearthFires.esm", "01CDAE:Dragonborn.esm", "01CDB2:Dragonborn.esm",
    "01CDB3:Dragonborn.esm"
  };

  std::vector<std::string> weaponTier4 = {
    "01CDB5:Dragonborn.esm", "01CDB9:Dragonborn.esm", "01CDBA:Dragonborn.esm",
    "014FCE:Dawnguard.esm",  "014FC3:Dawnguard.esm",  "080113:Dragonborn.esm",
    "080112:Dragonborn.esm", "01A578:Dragonborn.esm", "0179C9:Dragonborn.esm",
    "014FCD:Dawnguard.esm",  "014FCC:Dawnguard.esm",  "01CDB4:Dragonborn.esm",
    "01CDB8:Dragonborn.esm", "01CDB6:Dragonborn.esm", "01CDB7:Dragonborn.esm",
    "014FCB:Dawnguard.esm",  "014FCF:Dawnguard.esm",  "014FD0:Dawnguard.esm"
  };
  std::vector<std::string> weaponTier5 = { "039FB4:Dragonborn.esm",
                                           "01AEA4:Dragonborn.esm" };

  std::vector<std::string> armorTier2 = {
    "02AD31:Dragonborn.esm", "019ADE:Dawnguard.esm",  "03706A:Dragonborn.esm",
    "03706B:Dragonborn.esm", "03706C:Dragonborn.esm", "037065:Dragonborn.esm",
    "019AE3:Dawnguard.esm",  "019ADF:Dawnguard.esm",  "02AD33:Dragonborn.esm",
    "02AD34:Dragonborn.esm", "02AD32:Dragonborn.esm", "019AE1:Dawnguard.esm",
    "037066:Dragonborn.esm", "03705A:Dragonborn.esm"
  };
  std::vector<std::string> armorTier3 = {
    "03AB23:Dragonborn.esm", "01CD8C:Dragonborn.esm", "01CD99:Dragonborn.esm",
    "0292AE:Dragonborn.esm", "0050D0:Dawnguard.esm",  "039114:Dragonborn.esm",
    "01CD93:Dragonborn.esm", "01CD8B:Dragonborn.esm", "014758:Dawnguard.esm",
    "026235:Dragonborn.esm", "037B88:Dragonborn.esm", "037B8A:Dragonborn.esm",
    "026236:Dragonborn.esm", "039110:Dragonborn.esm", "01CD98:Dragonborn.esm",
    "03910E:Dragonborn.esm", "0191F3:Dawnguard.esm",  "00F3FA:Dawnguard.esm",
    "00F3F7:Dawnguard.esm",  "0292AC:Dragonborn.esm", "01CD97:Dragonborn.esm",
    "01CD8A:Dragonborn.esm", "037564:Dragonborn.esm", "01CD92:Dragonborn.esm",
    "01CD82:Dragonborn.esm", "01CD96:Dragonborn.esm", "0292AB:Dragonborn.esm",
    "014757:Dawnguard.esm",  "019AE1:Dawnguard.esm",  "039112:Dragonborn.esm",
    "01CD94:Dragonborn.esm", "0292AD:Dragonborn.esm", "026234:Dragonborn.esm",
    "0150B8:Dawnguard.esm",  "037B8E:Dragonborn.esm", "037B8C:Dragonborn.esm"
  };
  std::vector<std::string> armorTier4 = {
    "01CDA1:Dragonborn.esm", "00C814:Dawnguard.esm",  "0047DA:Dawnguard.esm",
    "00B5DB:Dawnguard.esm",  "0047D9:Dawnguard.esm",  "00B5DE:Dawnguard.esm",
    "026237:Dragonborn.esm", "00C817:Dawnguard.esm",  "00C816:Dawnguard.esm",
    "01CD9F:Dragonborn.esm", "01CD9E:Dragonborn.esm", "00C815:Dawnguard.esm",
    "01CDA0:Dragonborn.esm", "005759:Dawnguard.esm",  "01A51F:Dawnguard.esm"
  };
  std::vector<std::string> armorTier5 = {
    "039D2B:Dragonborn.esm", "02B0F9:Dragonborn.esm", "03D2AF:Dragonborn.esm",
    "039D1E:Dragonborn.esm", "0376F1:Dragonborn.esm"
  };
  std::vector<std::string> consumableTier3 = { "00353C:HearthFires.esm" };

  AddDLCItems(espmFiles, weaponTier3, LootboxItemType::Weapon, Tier::Tier3);
  AddDLCItems(espmFiles, weaponTier4, LootboxItemType::Weapon, Tier::Tier4);
  AddDLCItems(espmFiles, weaponTier5, LootboxItemType::Weapon, Tier::Tier5);
  AddDLCItems(espmFiles, armorTier2, LootboxItemType::Armor, Tier::Tier2);
  AddDLCItems(espmFiles, armorTier3, LootboxItemType::Armor, Tier::Tier3);
  AddDLCItems(espmFiles, armorTier4, LootboxItemType::Armor, Tier::Tier4);
  AddDLCItems(espmFiles, armorTier5, LootboxItemType::Armor, Tier::Tier5);
  AddDLCItems(espmFiles, consumableTier3, LootboxItemType::Consumable,
              Tier::Tier3);
}

const PieScript::LootTable& PieScript::GetLootTable() const
{
  return lootTable;
}

PieScript::Tier PieScript::AcknowledgeTier(int chance)
{
  Tier tier;
  if (chance <= TIER1_CHANCE) {
    tier = Tier::Tier1;
    return tier;
  } else if (chance <= (TIER1_CHANCE + TIER2_CHANCE)) {
    tier = Tier::Tier2;
    return tier;
  } else if (chance <= (TIER1_CHANCE + TIER2_CHANCE + TIER3_CHANCE)) {
    tier = Tier::Tier3;
    return tier;
  } else if (chance <=
             (TIER1_CHANCE + TIER2_CHANCE + TIER3_CHANCE + TIER4_CHANCE)) {
    tier = Tier::Tier4;
    return tier;
  } else {
    tier = Tier::Tier5;
    return tier;
  }
}

std::pair<PieScript::LootboxItemType, PieScript::Tier>
PieScript::AcknowledgeTypeAndTier(int weaponChance, int armorChance,
                                  int consumableChance, int nothingChance)
{
  int chance = GenerateRandomNumber(1, 100);
  LootboxItemType type;
  Tier tier;

  if (chance <= weaponChance && weaponChance != 0) {
    chance = GenerateRandomNumber(1, 1000);
    type = LootboxItemType::Weapon;
    tier = AcknowledgeTier(chance);
  } else if (chance <= (weaponChance + armorChance) && armorChance != 0) {
    chance = GenerateRandomNumber(1, 1000);
    type = LootboxItemType::Armor;
    tier = AcknowledgeTier(chance);
  } else if (chance <= (weaponChance + armorChance + consumableChance) &&
             consumableChance != 0) {
    chance = GenerateRandomNumber(1, 1000);
    type = LootboxItemType::Consumable;
    tier = AcknowledgeTier(chance);
  } else {
  }

  std::pair<LootboxItemType, Tier> tierAndType = std::make_pair(type, tier);
  return tierAndType;
}

uint32_t PieScript::GetSlotItem(int weaponChance, int armoryChacne,
                                int consumableChance, int nothingChance)
{
  std::pair<LootboxItemType, Tier> typeAndTier = AcknowledgeTypeAndTier(
    weaponChance, armoryChacne, consumableChance, nothingChance);
  if (typeAndTier.first != LootboxItemType::Nothing) {
    int item = GenerateRandomNumber(
      0, lootTable[typeAndTier.first][typeAndTier.second].size() - 1);
    return lootTable[typeAndTier.first][typeAndTier.second].at(item);
  }
  return 0;
}

void PieScript::Notify(MpActor& actor, const WorldState& worldState,
                       uint32_t formId, int count, bool silent)
{
  std::string type;
  std::stringstream ss;
  auto lookupRes = worldState.GetEspm().GetBrowser().LookupById(formId);
  auto recType = lookupRes.rec->GetType();

  if (recType == "WEAP") {
    type = "weapon";
  } else if (recType == "ARMO") {
    type = "armor";
  } else if (recType == "INGR") {
    type = "ingridient";
  } else if (recType == "LIGH") {
    type = "light";
  } else if (recType == "SLGM") {
    type = "soulgem";
  } else if (recType == "ALCH") {
    type = "potion";
  } else {
    throw std::runtime_error(fmt::format("Unexpected type {}", type));
  }

  ss << "[";
  ss << nlohmann::json({ { "formId", formId }, { "type", type } }).dump();
  ss << "," << static_cast<int>(count) << ","
     << (static_cast<bool>(silent) ? "true" : "false");
  ss << "]";
  std::string args = ss.str();
  (void)SpSnippet("SkympHacks", "AddItem", args.data()).Execute(&actor);
}

void PieScript::Play(MpActor& actor, const WorldState& worldState)
{
  uint32_t item1 = GetSlotItem(80, 10, 10, 0);
  uint32_t item2 = GetSlotItem(10, 80, 10, 0);
  uint32_t item3 = GetSlotItem(10, 10, 80, 0);
  uint32_t item4 = GetSlotItem(0, 0, 100, 0);

  if (item1) {
    actor.AddItem(item1, 1);
    Notify(actor, worldState, item1, 1, false);
  }
  if (item2) {
    actor.AddItem(item2, 1);
    Notify(actor, worldState, item2, 1, false);
  }
  if (item3) {
    actor.AddItem(item3, 1);
    Notify(actor, worldState, item3, 1, false);
  }
  if (item4) {
    actor.AddItem(item4, 1);
    Notify(actor, worldState, item4, 1, false);
  }
}

void PieScript::AddKitItems(MpActor& actor, const WorldState& worldState,
                            StarterKitType type)
{
  for (auto item : starterKitsMap[type]) {
    actor.AddItem(item, 1);
    Notify(actor, worldState, item, 1, false);
  }
}

void PieScript::AddStarterKitItems(MpActor& actor,
                                   const WorldState& worldState)
{
  int chance = GenerateRandomNumber(1, 100);
  if (chance <= StarterKitChance::ChefKitChance) {
    AddKitItems(actor, worldState, StarterKitType::ChefKit);
  } else if (chance <= (StarterKitChance::ChefKitChance +
                        StarterKitChance::LumberjackKitChance)) {
    AddKitItems(actor, worldState, StarterKitType::LumberjackKit);
  } else if (chance <= (StarterKitChance::ChefKitChance +
                        StarterKitChance::LumberjackKitChance +
                        StarterKitChance::MinerKitChance)) {
    AddKitItems(actor, worldState, StarterKitType::MinerKit);
  } else {
    AddKitItems(actor, worldState, StarterKitType::PrisonerKit);
  }
}

void PieScript::AddPatronStarterKitItems(MpActor& actor,
                                         const WorldState& worldState)
{
  AddKitItems(actor, worldState, StarterKitType::PatronKit);
}
