#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class SCRL final : public RecordHeader
{
public:
  static constexpr auto kType = "SCRL";

private:
  struct DATA
  {
    float weight;
  };

public:
  struct Data
  {
    DATA data;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(SCRL) == sizeof(RecordHeader));

}

#pragma pack(pop)
