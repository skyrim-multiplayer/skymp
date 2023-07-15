#pragma once
#include "ActorValue.h"
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class MGEF final : public RecordHeader
{
public:
  static constexpr auto kType = "MGEF";

  enum class EffectType : uint32_t
  {
    ValueMod = 0,
    Script,
    Dispel,
    CureDisease,
    Absorb,
    Dual,
    Calm,
    Demoralize,
    Frenzy,
    Disarm,
    CommandSummoned,
    Invisibility,
    Light,
    Lock = 15,
    Open,
    BoundWeapon,
    SummonCreature,
    DetectLife,
    Telekinesis,
    Paralysis,
    Reanimate,
    SoulTrap,
    TurnUndead,
    Guide,
    WerewolfFeed,
    CureParalysis,
    CureAddiction,
    CurePoison,
    Concussion,
    ValueAndParts,
    AccumulateMagnitude,
    Stagger,
    PeakValueMod,
    Cloak,
    Werewolf,
    SlowTime,
    Rally,
    EnchanceWeapon,
    SpawnHazard,
    Etherealize,
    Banish,
    SpawnScriptedRef,
    Disguise,
    GrabActor,
    VampireLord
  };
  static_assert(static_cast<std::underlying_type_t<EffectType>>(
                  EffectType::VampireLord) == 46);
  struct DATA
  {
    // primary actor value
    ActorValue primaryAV = espm::ActorValue::None;
    EffectType type;
  };

  struct Data
  {
    DATA data;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(MGEF) == sizeof(RecordHeader));

}

#pragma pack(pop)
