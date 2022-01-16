#include "PieScript.h"
#include "WorldState.h"
#include "MpActor.h"
#include <random>

std::mt19937 g_rng{ std::random_device{}() };

int GenerateRandomNumber(int leftBound, int rightBound)
{
  std::uniform_int_distribution<> distr(leftBound, rightBound);
  return distr(g_rng);
}

PieScript::PieScript()
{
  lootTable = {
    { LootboxItemType::Weapon,
      {
        {
          Tier::Tier1,
          { 0x0001397E, 0x00013790, 0x00013981, 0x0002C66F, 0x0001CB64,
            0x00012EB7, 0x00013980, 0x000CADE9, 0x0002C672, 0x0002E6D1,
            0x0001359D, 0x00013982, 0x000CC829, 0x000236A5, 0x000302CD },
        },
        { Tier::Tier2,
          { 0x00013986, 0x00013983, 0x0001398A, 0x00013997, 0x00013998,
            0x000139A1, 0x0001399C, 0x0001398E, 0x0001398B, 0x00013992,
            0x00013989, 0x00013984, 0x00013996, 0x00013993, 0x0001399A,
            0x0001399F, 0x000139A0, 0x00013991, 0x0001398C, 0x0010AA19,
            0x00013987, 0x00013988, 0x00013999, 0x00013994, 0x0001399E,
            0x0001399B, 0x000139A2, 0x0001398F, 0x00013990, 0x0010C6FB } },
        { Tier::Tier3, {} },
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
            0x00056a9D, 0x0006F39B, 0x0001B3A0, 0x00013914, 0x0005C06C,
            0x0006c1d9, 0x0006ff43, 0x0006C1D8, 0x0003452E, 0x000D191F,
            0x0003452F, 0x000C36E9 } },
      } },
    { LootboxItemType::Consumable,
      {
        { Tier::Tier1, { 0x0004B0BA, 0x00034CDF, 0x0005076E, 0x0001D4EC } },
        { Tier::Tier2,
          { 0x0006AC4A, 0x0001B3BD, 0x00064B2E, 0x00064B2F, 0x00023D77 } },
        { Tier::Tier3, { 0x0000353C } }, // change to dlc id 0xXX00353c
        { Tier::Tier4, { 0x0003EADE } },
        { Tier::Tier5, { 0x002E504 } },
      } }
  };
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
    chance = GenerateRandomNumber(1, 100);
    type = LootboxItemType::Weapon;
    tier = AcknowledgeTier(chance);
  } else if (chance <= (weaponChance + armorChance) && armorChance != 0) {
    chance = GenerateRandomNumber(1, 100);
    type = LootboxItemType::Armor;
    tier = AcknowledgeTier(chance);
  } else if (chance <= (weaponChance + armorChance + consumableChance) &&
             consumableChance != 0) {
    chance = GenerateRandomNumber(1, 100);
    type = LootboxItemType::Consumable;
    tier = AcknowledgeTier(chance);
  } else {
    type = LootboxItemType::Nothing;
  }

  std::pair<LootboxItemType, Tier> tierAndType = std::make_pair(type, tier);
  return tierAndType;
}

uint32_t PieScript::GetSlotItem(int weaponChance, int armoryChacne,
                                int consumableChance, int nothingChance)
{
  std::pair<LootboxItemType, Tier> typeAndTier = AcknowledgeTypeAndTier(
    weaponChance, armoryChacne, consumableChance, nothingChance);
  int item = GenerateRandomNumber(
    0, lootTable[typeAndTier.first][typeAndTier.second].size() - 1);
  return lootTable[typeAndTier.first][typeAndTier.second][item];
  ;
}

void PieScript::Play(WorldState* worldState, uint32_t id)
{
  MpActor& actor = worldState->GetFormAt<MpActor>(id);
  actor.AddItem(GetSlotItem(80, 10, 8, 2), 1);
  actor.AddItem(GetSlotItem(10, 80, 8, 2), 1);
  actor.AddItem(GetSlotItem(10, 50, 30, 10), 1);
  actor.AddItem(GetSlotItem(0, 0, 94, 6), 1);
}
