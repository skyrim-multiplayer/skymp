#pragma once
#include "RecordHeader.h"
#include "CTDA.h"
#include <optional>

#pragma pack(push, 1)

namespace espm {

class FACT final : public RecordHeader
{
public:
  static constexpr auto kType = "FACT";

  enum class Flags : uint32_t
  {
    HiddenFromPC = 0x1,
    SpecialCombat = 0x2,
    TrackCrime = 0x40,
    IgnoreMurder = 0x80,
    IgnoreAssault = 0x100,
    IgnoreStealing = 0x200,
    IgnoreTrespass = 0x400,
    DoNotReportCrimesAgainstMembers = 0x800,
    CrimeGoldUseDefaults = 0x1000,
    IgnorePickpocket = 0x2000,
    Vendor = 0x4000,
    CanBeOwner = 0x8000,
    IgnoreWerewolf = 0x10000
  };

  enum class CombatState : int32_t
  {
    Neutral = 0,
    Enemy = 1,
    Ally = 2,
    Friend = 3
  };

  struct InterfactionRelation
  {
    uint32_t factionFormId = 0;
    int32_t modUnused = 0;
    CombatState combat;
  };
  static_assert(sizeof(InterfactionRelation) == 12);

  // This struct can have 3 different layouts(12, 16, 20 bytes).
  // Used 12 bytes version.
  struct CrimeGold
  {
    uint8_t arrest = 0;
    uint8_t attackOnSight = 0;
    uint16_t murder = 0;
    uint16_t assault = 0;
    uint16_t trespass = 0;
    uint16_t pickpocket = 0;
    uint16_t unused = 0;
  };
  static_assert(sizeof(CrimeGold) == 12);

  struct Data
  {
    std::optional<std::string> fullName;
    std::vector<InterfactionRelation> interfactionRelations;
    Flags flags;
    std::optional<uint32_t> prisonMarker = 0;
    std::optional<uint32_t> followerWaitMarker = 0;
    std::optional<uint32_t> evidenceChest = 0;
    std::optional<uint32_t> playerBelongingsChest = 0;
    std::optional<uint32_t> crimeGroup = 0;
    std::optional<uint32_t> jailOutfit = 0;
    std::optional<CrimeGold> crimeGold;
    // ranks skipped
    // vendor items skipped
    std::vector<CTDA> conditions;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(FACT) == sizeof(RecordHeader));

}

#pragma pack(pop)
