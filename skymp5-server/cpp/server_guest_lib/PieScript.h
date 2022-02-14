#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class MpActor;
class WorldState;

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
  PieScript(const std::vector<std::string>& espmFiles);

public:
  using LootTable =
    std::unordered_map<LootboxItemType,
                       std::unordered_map<Tier, std::vector<uint32_t>>>;
  const LootTable& GetLootTable() const;
  void Play(MpActor& actor, const WorldState& worldState);
  void AddStarterKitItems(MpActor& actor, const WorldState& worldState);
  void AddPatronStarterKitItems(MpActor& actor, const WorldState& worldState);

private:
  Tier AcknowledgeTier(int chance);
  uint32_t GetSlotItem(int weaponChance, int armoryChance,
                       int consumableChance, int nothingChance);
  std::pair<LootboxItemType, Tier> AcknowledgeTypeAndTier(int weaponChance,
                                                          int armoryChance,
                                                          int consumableChance,
                                                          int nothingChance);
  void AddDLCItems(const std::vector<std::string>& espmFiles,
                   const std::vector<std::string>& items, LootboxItemType type,
                   Tier tier);
  void AddKitItems(MpActor& actor, const WorldState& worldState,
                   StarterKitType starterKitType);
  void Notify(MpActor& actor, const WorldState& worldState, uint32_t formId,
              int count, bool silent);

private:
  LootTable lootTable;
  std::unordered_map<StarterKitType, std::vector<uint32_t>> starterKitsMap;

  const int TIER1_CHANCE = 600;
  const int TIER2_CHANCE = 220;
  const int TIER3_CHANCE = 150;
  const int TIER4_CHANCE = 25;
  const int TIER5_CHANCE = 5;
};
