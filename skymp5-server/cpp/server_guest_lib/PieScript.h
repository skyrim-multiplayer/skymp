#pragma once
#include "MpChangeForms.h"
#include "WorldState.h"
#include <unordered_map>
#include <utility>

class PieScript
{
public:
  enum class LootboxItemType
  {
    Weapon = 0,
    Armor,
    Consumable,
    Nothing
  };

  enum class Tier
  {
    Tier1 = 1,
    Tier2,
    Tier3,
    Tier4,
    Tier5
  };

  using LootTable =
    std::unordered_map<LootboxItemType,
                       std::unordered_map<Tier, std::vector<uint32_t>>>;

public:
  PieScript(std::vector<std::string> espmFiles);

public:
  const LootTable& GetLootTable() const;
  void Play(MpActor* actor);

private:
  Tier AcknowledgeTier(int chance);
  uint32_t GetSlotItem(int weaponChance, int armoryChance,
                       int consumableChance, int nothingChance);
  std::pair<LootboxItemType, Tier> AcknowledgeTypeAndTier(int weaponChance,
                                                          int armoryChance,
                                                          int consumableChance,
                                                          int nothingChance);

private:
  LootTable lootTable;

  const int TIER1_CHANCE = 100;
  const int TIER2_CHANCE = 20;
  const int TIER3_CHANCE = 10;
  const int TIER4_CHANCE = 9;
  const int TIER5_CHANCE = 1;
};
