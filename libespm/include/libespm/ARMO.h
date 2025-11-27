#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class ARMO final : public RecordHeader
{
public:
  static constexpr auto kType = "ARMO";

  struct Data
  {
    uint32_t baseRatingX100 = 0;
    uint32_t baseValue = 0;
    float weight = 0;
    uint32_t enchantmentFormId = 0;
    uint32_t equipSlotId = 0; // only for shields

    struct
    {
      bool present = false;
      uint32_t bodyPartFlags = 0;
      uint32_t skill = 0;
    } bodt, bod2;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(ARMO) == sizeof(RecordHeader));

}

#pragma pack(pop)
