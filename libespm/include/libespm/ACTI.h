#pragma once
#include "RecordHeader.h"
#include "ScriptData.h"

#pragma pack(push, 1)

namespace espm {

class ACTI : public RecordHeader
{
public:
  static constexpr auto kType = "ACTI";

  struct Data
  {
    ScriptData scriptData;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(ACTI) == sizeof(RecordHeader));

}

#pragma pack(pop)
