#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class MpActor;

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

  enum class StarterKitType
  {
    ChefKit,
    MinerKit,
    PrisonerKit,
    LumberjackKit,
    PatronKit
  };

private:
  enum StarterKitChance
  {
    ChefKitChance = 25,
    MinerKitChance = 25,
    PrisonerKitChance = 25,
    LumberjackKitChance = 25
  };

public:
  PieScript(std::vector<std::string> espmFiles);

public:
  using LootTable =
    std::unordered_map<LootboxItemType,
                       std::unordered_map<Tier, std::vector<uint32_t>>>;
  const LootTable& GetLootTable() const;
  void Play(MpActor* actor);
  void GetStarterKitItems(MpActor* actor);
  void GetPatronStarterKitItems(MpActor* actor);

private:
  Tier AcknowledgeTier(int chance);
  uint32_t GetSlotItem(int weaponChance, int armoryChance,
                       int consumableChance, int nothingChance);
  std::pair<LootboxItemType, Tier> AcknowledgeTypeAndTier(int weaponChance,
                                                          int armoryChance,
                                                          int consumableChance,
                                                          int nothingChance);
  void AddDLCItems(std::vector<std::string> espmFiles,
                   std::vector<std::string> items, LootboxItemType type,
                   Tier tier);
  void AddStarterKitItems(MpActor* actor, StarterKitType starterKitType);

private:
  LootTable lootTable;
  std::unordered_map<StarterKitType, std::array<uint32_t, 5>> starterKitsMap;

  const int TIER1_CHANCE = 60;
  const int TIER2_CHANCE = 20;
  const int TIER3_CHANCE = 10;
  const int TIER4_CHANCE = 9;
  const int TIER5_CHANCE = 1;
};
