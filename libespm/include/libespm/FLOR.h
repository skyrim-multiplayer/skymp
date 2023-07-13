#pragma once
#include "RecordHeader.h"
#include "TREE.h"

#pragma pack(push, 1)

namespace espm {

class FLOR final : public RecordHeader
{
public:
  static constexpr auto kType = "FLOR";

  using Data = TREE::Data;

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(TREE) == sizeof(RecordHeader));

}

#pragma pack(pop)
