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

    bool hasBODT = false;
    uint32_t BODT_flags = 0;
    uint32_t BODT_skill = 0;

    bool hasBOD2 = false;
    uint32_t BOD2_flags = 0;
    uint32_t BOD2_skill = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(ARMO) == sizeof(RecordHeader));

}

#pragma pack(pop)