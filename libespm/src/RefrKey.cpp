#include "libespm/RefrKey.h"
#include "libespm/EspmUtils.h"

namespace espm {

RefrKey::RefrKey(uint32_t cellOrWorld, int16_t x, int16_t y)
  : value(InitValue(cellOrWorld, x, y))
{
}

RefrKey::operator uint64_t() const noexcept
{
  return value;
}

uint64_t InitValue(uint32_t cellOrWorld, int16_t x, int16_t y)
{
    union
    {
      struct
      {
        int16_t x, y;
      };
      uint32_t unsignedInt;
    } myUnion;
    myUnion.x = x;
    myUnion.y = y;
    return utils::MakeUInt64(cellOrWorld, myUnion.unsignedInt);
}

}
