#pragma once
#include "Equipment.h"
#include "FormDesc.h"
#include "Inventory.h"
#include "Look.h"
#include "NiPoint3.h"
#include <cstdint>
#include <optional>
#include <string>

class MpObjectReference;
class WorldState;

class MpChangeFormREFR
{
public:
  enum RecType
  {
    REFR = 0,
    ACHR = 1,
  };

  int recType = RecType::REFR;
  FormDesc formDesc;
  FormDesc baseDesc;
  NiPoint3 position = { 0, 0, 0 };
  NiPoint3 angle = { 0, 0, 0 };
  uint32_t worldOrCell = 0;
  Inventory inv;
  bool isHarvested = false;
  bool isOpen = false;
  bool baseContainerAdded = false;
  uint64_t nextRelootDatetime = 0;
  bool isDisabled = false;

  bool isRaceMenuOpen = false;

  // 'lookDump' and 'equipmentDump' can be empty. it means nullopt.
  // "unexisting" equipment and equipment with zero entries are different
  // values in skymp due to poor designs
  std::string lookDump, equipmentDump;
};

class MpChangeForm : public MpChangeFormREFR
{
public:
};