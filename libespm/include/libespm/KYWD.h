#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class KYWD final : public espm::RecordHeader
{
public:
  static constexpr auto kType = "KYWD";

  struct Data
  {
    const char* editorId = "";
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(KYWD) == sizeof(espm::RecordHeader));

}

#pragma pack(pop)
