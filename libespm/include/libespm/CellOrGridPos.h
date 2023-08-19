#pragma once
#include <cstdint>

#pragma pack(push, 1)

namespace espm {

union CellOrGridPos
{
  uint32_t cellId = 0;
  struct
  {
    int16_t y;
    int16_t x;
  } pos;
};

}

#pragma pack(pop)
