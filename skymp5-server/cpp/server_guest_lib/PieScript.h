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
private:
  enum StarterKitChance
  {
    ChefKitChance = 25,
    MinerKitChance = 25,
    PrisonerKitChance = 25,
    LumberjackKitChance = 25
  };

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

  enum EdibleItems
  {
    kApplePieId = 0x082b9671,
    kStareterKitPie = 0x030009DB,
    kPatronStarterKitPie = 0x00064B30,
    kWardrobePie = 0x082DD28a,
    spellBookOfSummonedBattleAxe = 0x07A45089,
    spellBookOfSummonedBow = 0x07A45088,
    spellBookOfSummonedDagger = 0x07A4A18F,
    spellBookOfSummonedSword = 0x07A4A18D
  };

public:
  PieScript(const std::vector<std::string>& espmFiles);
  void Play(MpActor& actor, const WorldState& worldState, uint32_t itemBaseId);

private:
  using LootboxTable =
    std::unordered_map<LootboxItemType,
                       std::unordered_map<Tier, std::vector<uint32_t>>>;
  void AddPieItems(MpActor& actor, const WorldState& worldState);
  void AddStarterKitItems(MpActor& actor, const WorldState& worldState);
  void AddPatronStarterKitItems(MpActor& actor, const WorldState& worldState);
  Tier AcknowledgeTier(uint32_t chance);
  uint32_t GetSlotItem(uint32_t weaponChance, uint32_t armoryChance,
                       uint32_t consumableChance, uint32_t nothingChance);
  std::pair<LootboxItemType, Tier> AcknowledgeTypeAndTier(
    uint32_t weaponChance, uint32_t armoryChance, uint32_t consumableChance,
    uint32_t nothingChance);
  void AddDLCItems(const std::vector<std::string>& espmFiles,
                   const std::vector<std::string>& items, LootboxItemType type,
                   Tier tier);
  void AddKitItems(MpActor& actor, const WorldState& worldState,
                   StarterKitType starterKitType);
  void Notify(MpActor& actor, const WorldState& worldState, uint32_t formId,
              uint32_t count, bool silent);

private:
  LootboxTable lootboxTable;
  std::unordered_map<StarterKitType, std::vector<uint32_t>> starterKitsMap;
  std::unordered_map<uint32_t, uint32_t> miscLootTable;

  const uint32_t TIER1_CHANCE = 600;
  const uint32_t TIER2_CHANCE = 220;
  const uint32_t TIER3_CHANCE = 150;
  const uint32_t TIER4_CHANCE = 25;
  const uint32_t TIER5_CHANCE = 5;
};
