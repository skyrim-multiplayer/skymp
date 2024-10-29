#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class SPEL final : public RecordHeader
{
public:
  static constexpr auto kType = "SPEL";

  enum class SpellType
  {
    Spell = 0x00,
    Disease = 0x01,
    Power = 0x02,
    LesserPower = 0x03,
    Ability = 0x04,
    Poison = 0x05,
    Addiction = 0x0A,
    Voice = 0x0B
  };

  enum class SpellFlags
  {
    None = 0,
    NotAutoCalculate = 0x1,
    Unknown1 = 0x10000,
    PCStartSpell = 0x20000,
    Unknown2 = 0x40000,
    AreaEffectIgnoresLineofSight = 0x80000,
    IgnoreResistance = 0x100000,
    DisallowSpellAbsorbOrReflect = 0x200000,
    Unknown3 = 0x400000,
    NoDualCastModifications = 0x800000,
  };

  enum class CastType
  {
    ConstantEffect = 0x00,
    FireAndForget = 0x01,
    Concentration = 0x02,
  };

  enum class Delivery
  {
    Self = 0x00,
    Contact = 0x01,
    Aimed = 0x02,
    TargetActor = 0x03,
    TargetLocation = 0x04,
  };

  struct SPITData
  {
    // if auto-calc, game bases cost on the sum of the effect costs
    uint32_t spellCost = 0;
    SpellFlags flags = SpellFlags::None;
    SpellType type = SpellType::Spell;

    // if auto-calc, game uses the maximum of the casting times of the effects
    // instead
    float chargeTime = 0.f;
    CastType castType = CastType::ConstantEffect;
    Delivery delivery = Delivery::Self;

    // determines minimum duration of a Concentrated spell
    float castDuration = 0.f;

    // valid for Delivery TargetActor or TargetLocation
    float range = 0.f;

    uint32_t perkFormId = 0;
  };

  static_assert(sizeof(SPITData) == 36);

  struct EFIT
  {
    float magnitude = 0.f;
    uint32_t areaOfEffect = 0;
    uint32_t duration = 0;
  };

  static_assert(sizeof(EFIT) == 12);

  struct Effect
  {
    uint32_t effectFormId = 0;
    const EFIT* effectItem = nullptr;
  };

  struct Data
  {
    const SPITData* spellItem = nullptr;
    std::vector<Effect> effects{};
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(SPEL) == sizeof(RecordHeader));

}

#pragma pack(pop)
