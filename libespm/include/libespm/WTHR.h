#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class WTHR final : public RecordHeader
{
public:
  static constexpr auto kType = "WTHR";

  struct Data
  {
    // Assuming similar structure to SLGM for now
    float weight;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(WTHR) == sizeof(RecordHeader));

}

#pragma pack(pop)