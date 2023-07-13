#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class OTFT final : public RecordHeader
{
public:
  static constexpr auto kType = "OTFT";

  struct Data
  {
    const uint32_t* formIds = nullptr;
    uint32_t count = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(OTFT) == sizeof(RecordHeader));

}

#pragma pack(pop)
