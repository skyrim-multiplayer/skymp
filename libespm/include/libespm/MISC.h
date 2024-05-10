#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class MISC final : public RecordHeader
{
public:
  static constexpr auto kType = "MISC";

  struct Data
  {
    float weight;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(MISC) == sizeof(RecordHeader));

}

#pragma pack(pop)
