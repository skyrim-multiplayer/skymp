#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class SHOU final : public RecordHeader
{
public:
  static constexpr auto kType = "SHOU";

  struct Data
  {
    float recoveryTime[3];
    uint32_t wordOfPower[3];
    uint32_t spell[3];
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(SHOU) == sizeof(RecordHeader));

}

#pragma pack(pop)
