#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class BOOK : public RecordHeader
{
public:
  static constexpr auto kType = "BOOK";

  enum Flags : uint8_t
  {
    None = 0,
    TeachesSkill = 0x01,
    CantbeTaken = 0x02,
    TeachesSpell = 0x04,
    AlreadyRead = 0x08,
  };

  struct Data
  {
    [[nodiscard]] bool IsFlagSet(Flags flag) const noexcept;

    Flags flags = Flags::None;
    uint32_t spellOrSkillFormId = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(BOOK) == sizeof(RecordHeader));

}

#pragma pack(pop)
