#pragma once
#include "ActorValues.h"
#include "Appearance.h"
#include "DynamicFields.h"
#include "Equipment.h"
#include "FormDesc.h"
#include "Inventory.h"
#include "LocationalData.h"
#include "NiPoint3.h"
#include <cstdint>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <tuple>

class MpObjectReference;
class WorldState;

struct LearnedSpells
{
  using Data = std::set<uint32_t>;

  void LearnSpell(Data::key_type baseId);

  [[nodiscard]] size_t Count() const noexcept;

  [[nodiscard]] bool IsSpellLearned(Data::key_type baseId) const;

  std::vector<Data::key_type> GetLearnedSpells() const;

  friend bool operator==(const LearnedSpells& lhs, const LearnedSpells& rhs)
  {
    return lhs._learnedSpellIds == rhs._learnedSpellIds;
  }

  friend bool operator!=(const LearnedSpells& lhs, const LearnedSpells& rhs)
  {
    return !(lhs == rhs);
  }

  friend bool operator<(const LearnedSpells& lhs, const LearnedSpells& rhs)
  {
    return lhs._learnedSpellIds < rhs._learnedSpellIds;
  }

private:
  Data _learnedSpellIds{};
};

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
  FormDesc worldOrCellDesc;
  Inventory inv;
  LearnedSpells learnedSpells;

  bool isHarvested = false;
  bool isOpen = false;
  bool baseContainerAdded = false;
  uint64_t nextRelootDatetime = 0;
  bool isDisabled = false;
  int32_t profileId = -1;

  bool isRaceMenuOpen = false;
  bool isDead = false;
  bool consoleCommandsAllowed = false;

  // 'appearanceDump' and 'equipmentDump' can be empty. it means nullopt.
  // "unexisting" equipment and equipment with zero entries are different
  // values in skymp due to poor design
  std::string appearanceDump, equipmentDump;
  ActorValues actorValues;
  LocationalData spawnPoint = { { 133857, -61130, 14662 },
                                { 0.f, 0.f, 72.f },
                                FormDesc::Tamriel() };
  float spawnDelay = 5.0f;

  // Please update 'ActorTest.cpp' when adding new Actor-related rows

  DynamicFields dynamicFields;

  auto ToTuple() const
  {
    return std::make_tuple(
      recType, formDesc, baseDesc, position.x, position.y, position.z, angle.x,
      angle.y, angle.z, worldOrCellDesc, inv.ToJson(), isHarvested, isOpen,
      baseContainerAdded, nextRelootDatetime, isDisabled, profileId,
      isRaceMenuOpen, isDead, consoleCommandsAllowed, appearanceDump,
      equipmentDump, actorValues.ToTuple(), spawnPoint, dynamicFields,
      spawnDelay, learnedSpells);
  }

  static nlohmann::json ToJson(const MpChangeFormREFR& changeForm);
  static MpChangeFormREFR JsonToChangeForm(simdjson::dom::element& element);
};

#define MpChangeForm MpChangeFormREFR

inline bool operator==(const MpChangeFormREFR& lhs,
                       const MpChangeFormREFR& rhs)
{
  return lhs.ToTuple() == rhs.ToTuple();
}

inline bool operator!=(const MpChangeFormREFR& lhs,
                       const MpChangeFormREFR& rhs)
{
  return !(lhs == rhs);
}

inline bool operator<(const MpChangeFormREFR& lhs, const MpChangeFormREFR& rhs)
{
  return lhs.ToTuple() < rhs.ToTuple();
}
