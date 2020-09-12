#pragma once
#include "Inventory.h"
#include "Look.h"
#include <cstdint>
#include <string>

class MpObjectReference;
class WorldState;

class MpChangeFormREFR
{
public:
  int32_t id = 0;
  uint32_t shortFormId = 0;
  std::string file;
  float pos[3] = { 0, 0, 0 }; //
  float rot[3] = { 0, 0, 0 }; //
  uint32_t worldOrCell = 0;   //
  Inventory inv;              //
  bool isHarvested = false;   //
  bool isOpen = false;        //
  uint64_t nextRelootDatetime = 0;
};

class MpChangeFormACHR
{
public:
  bool isRaceMenuOpen = false;
  std::string lookDump;
  std::string equipmentDump;
};

class MpChangeForm
  : public MpChangeFormREFR
  , public MpChangeFormACHR
{
public:
  static void Load(MpChangeForm source, WorldState* parentWorldState);

  static MpChangeForm Save(MpObjectReference* refr,
                           WorldState* parentWorldState);

  std::string GetInventoryDump() const;
  void SetInventoryDump(const std::string& inventoryDump);
};