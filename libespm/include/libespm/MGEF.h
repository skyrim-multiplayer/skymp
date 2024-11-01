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

  enum class Flags : uint32_t
  {
    Hostile = 0x00000001,
    Recover = 0x00000002,
    Detrimental = 0x00000004,
    SnapToNavmesh = 0x00000008,
    NoHitEvent = 0x00000010,
    DispelEffects = 0x00000100,
    NoDuration = 0x00000200,
    NoMagnitude = 0x00000400,
    NoArea = 0x00000800,
    FXPersist = 0x00001000,
    GoryVisual = 0x00004000,
    HideInUI = 0x00008000,
    NoRecast = 0x00020000,
    PowerAffectsMagnitude = 0x00200000,
    PowerAffectsDuration = 0x00400000,
    Painless = 0x04000000,
    NoHitEffect = 0x08000000,
    NoDeathDispel = 0x10000000
  };

  struct DATA
  {
    // primary actor value
    Flags flags;
    ActorValue primaryAV = espm::ActorValue::None;
    EffectType effectType;

    [[nodiscard]] inline bool IsFlagSet(Flags flag) const
    {
      return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) ==
        static_cast<uint32_t>(flag);
    }
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
