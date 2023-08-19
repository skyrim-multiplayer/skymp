#pragma once
#include <cstdint>

#pragma pack(push, 1)

namespace espm {

struct ObjectBounds
{
  int16_t pos1[3] = { 0, 0, 0 };
  int16_t pos2[3] = { 0, 0, 0 };
};

static_assert(sizeof(ObjectBounds) == 12);

}

#pragma pack(pop)
