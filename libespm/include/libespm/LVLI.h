#pragma once
#include "LeveledListBase.h"

#pragma pack(push, 1)

namespace espm {

class LVLI final : public LeveledListBase
{
public:
  static constexpr auto kType = "LVLI";
};

static_assert(sizeof(LVLI) == sizeof(RecordHeader));

}

#pragma pack(pop)
