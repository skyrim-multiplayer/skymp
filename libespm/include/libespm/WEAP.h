#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class WEAP : public RecordHeader
{
public:
  static constexpr auto kType = "WEAP";

  struct WeapData
  {
    int32_t value = 0;
    float weight = 0.f;
    int16_t damage = 0;
  };

  static_assert(sizeof(WeapData) == 10);

  enum class AnimType : uint8_t
  {
    Other = 0,
    OneHandSword = 1,
    OneHandDagger = 2,
    OneHandAxe = 3,
    OneHandMace = 4,
    TwoHandSword = 5,
    TwoHandAxe = 6,
    Bow = 7,
    Staff = 8,
    Crossbow = 9
  };
  static_assert(sizeof(AnimType) == 1);

  struct DNAM
  {
    AnimType animType = AnimType::Other;
    uint8_t unknown01 = 0;
    uint16_t unknown02 = 0;
    float speed = 0.f;
    float reach = 0.f;
    // 0C: flags, etc
  };
  static_assert(sizeof(DNAM) == 0x0c);

  struct Data
  {
    const WeapData* weapData = nullptr;
    const DNAM* weapDNAM = nullptr;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(WEAP) == sizeof(RecordHeader));

}

#pragma pack(pop)
