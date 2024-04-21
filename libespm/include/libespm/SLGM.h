#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class SLGM final : public RecordHeader
{
public:
  static constexpr auto kType = "SLGM";

  struct Data
  {
    float weight;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(SLGM) == sizeof(RecordHeader));

}

#pragma pack(pop)
