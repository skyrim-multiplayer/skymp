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

  struct Data
  {
    SpellType type = SpellType::Spell;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(SPEL) == sizeof(RecordHeader));

}

#pragma pack(pop)
