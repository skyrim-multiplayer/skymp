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
  enum EdibleItems
  {
    kWardrobePie = 0x082DD28a,
  };

public:
  SweetPieScript(const std::vector<std::string>& espmFiles);
  void Play(MpActor& actor, WorldState& worldState, uint32_t itemBaseId);

private:
  void AddItem(MpActor& actor, const WorldState& worldState,
               uint32_t itemBaseId, uint32_t count);
  void AddPieItems(MpActor& actor, const WorldState& worldState);
  void Notify(MpActor& actor, const WorldState& worldState, uint32_t formId,
              uint32_t count, bool silent);
  void EquipItem(MpActor& actor, uint32_t formId, bool preventRemoval = false,
                 bool silent = false);

private:
  std::unordered_map<uint32_t, std::vector<uint32_t>> miscLootTable;
  std::unordered_map<uint32_t, SweetPieBoundWeapon> bookBoundWeapons;
};
