#pragma once
#include "Effects.h"
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class ALCH final : public RecordHeader
{
public:
  static constexpr auto kType = "ALCH";

  struct Data
  {
    std::vector<Effects::Effect> effects;
    float weight;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(ALCH) == sizeof(RecordHeader));

}

#pragma pack(pop)
