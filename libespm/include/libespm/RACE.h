#pragma once
#include "RecordHeader.h"
#include <set>

#pragma pack(push, 1)

namespace espm {

class RACE final : public RecordHeader
{
public:
  static constexpr auto kType = "RACE";

  struct Data
  {
    float startingHealth = 0.f;
    float startingMagicka = 0.f;
    float startingStamina = 0.f;
    float healRegen = 0.f;
    float magickaRegen = 0.f;
    float staminaRegen = 0.f;
    float unarmedDamage = 0.f;
    float unarmedReach = 0.f;

    std::set<uint32_t> spells = {};
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(RACE) == sizeof(RecordHeader));

}

#pragma pack(pop)
