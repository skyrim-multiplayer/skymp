#pragma once
#include "LeveledListBase.h"

#pragma pack(push, 1)

namespace espm {

class LVLN final : public LeveledListBase
{
public:
  static constexpr auto kType = "LVLN";
};

static_assert(sizeof(LVLN) == sizeof(RecordHeader));

}

#pragma pack(pop)
