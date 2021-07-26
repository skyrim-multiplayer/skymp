#pragma once
#include "DynamicFields.h"
#include "Equipment.h"
#include "FormDesc.h"
#include "Inventory.h"
#include "Look.h"
#include "NiPoint3.h"
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>

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
  int32_t profileId = -1;

  bool isRaceMenuOpen = false;

  // 'lookDump' and 'equipmentDump' can be empty. it means nullopt.
  // "unexisting" equipment and equipment with zero entries are different
  // values in skymp due to poor design
  std::string lookDump, equipmentDump;

  // Much attention to 'MpActor::GetChangeForm()' and 'ActorTest.cpp' when
  // adding new Actor-related rows

  DynamicFields dynamicFields;
};

class MpChangeForm : public MpChangeFormREFR
{
public:
  auto ToTuple() const
  {
    return std::make_tuple(recType, formDesc, baseDesc, position.x, position.y,
                           position.z, angle.x, angle.y, angle.z, worldOrCell,
                           inv.ToJson(), isHarvested, isOpen,
                           baseContainerAdded, nextRelootDatetime, isDisabled,
                           profileId, isRaceMenuOpen, lookDump, equipmentDump);
  }

  static nlohmann::json ToJson(const MpChangeForm& changeForm);
  static MpChangeForm JsonToChangeForm(simdjson::dom::element& element);
};

inline bool operator==(const MpChangeForm& lhs, const MpChangeForm& rhs)
{
  return lhs.ToTuple() == rhs.ToTuple();
}

inline bool operator!=(const MpChangeForm& lhs, const MpChangeForm& rhs)
{
  return !(lhs == rhs);
}

inline bool operator<(const MpChangeForm& lhs, const MpChangeForm& rhs)
{
  return lhs.ToTuple() < rhs.ToTuple();
}

inline std::ostream& operator<<(std::ostream& os,
                                const MpChangeForm& changeForm)
{
  return os << "{" << changeForm.formDesc.ToString() << ", "
            << "[" << changeForm.position.x << ", " << changeForm.position.y
            << ", " << changeForm.position.z << "] "
            << "}";
}
