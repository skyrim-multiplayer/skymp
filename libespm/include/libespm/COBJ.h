#pragma once
#include "RecordHeader.h"
#include "libespm/CTDA.h"

#pragma pack(push, 1)

namespace espm {

class COBJ final : public RecordHeader
{
public:
  static constexpr auto kType = "COBJ";

  struct InputObject
  {
    uint32_t formId = 0;
    uint32_t count = 0;
  };
  static_assert(sizeof(InputObject) == 8);

  struct Data
  {
    std::vector<InputObject> inputObjects;
    uint32_t outputObjectFormId = 0;
    uint32_t benchKeywordId = 0;
    std::vector<CTDA> conditions;
    uint32_t outputCount = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(COBJ) == sizeof(RecordHeader));

}

#pragma pack(pop)
