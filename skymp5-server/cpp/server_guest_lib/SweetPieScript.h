#pragma once
#include "SweetPieBoundWeapon.h"
#include <array>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class MpActor;
class WorldState;

class SweetPieScript
{
private:
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
  };

public:
  SweetPieScript(const std::vector<std::string>& espmFiles);
  void Play(MpActor& actor, WorldState& worldState, uint32_t itemBaseId);

private:
  using LootboxTable =
    std::unordered_map<LootboxItemType,
                       std::unordered_map<Tier, std::vector<uint32_t>>>;
  void AddItem(MpActor& actor, const WorldState& worldState,
               uint32_t itemBaseId, uint32_t count);
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
  void EquipItem(MpActor& actor, uint32_t formId, bool preventRemoval = false,
                 bool silent = false);

private:
  LootboxTable lootboxTable;
  std::unordered_map<StarterKitType, std::vector<uint32_t>> starterKitsMap;
  std::unordered_map<uint32_t, std::vector<uint32_t>> miscLootTable;
  std::unordered_map<uint32_t, SweetPieBoundWeapon> bookBoundWeapons;

  constexpr static uint32_t kTier1Chance = 600;
  constexpr static uint32_t kTier2Chance = 220;
  constexpr static uint32_t kTier3Chance = 150;
  constexpr static uint32_t kTier4Chance = 25;
  constexpr static uint32_t kTier5Chance = 5;
  constexpr static uint32_t kChefKitChance = 25;
  constexpr static uint32_t kMinerKitChance = 25;
  constexpr static uint32_t kPrisonerKitChance = 25;
  constexpr static uint32_t kLumberjackKitChance = 25;
};
