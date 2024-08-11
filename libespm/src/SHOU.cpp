#include "libespm/SHOU.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

SHOU::Data SHOU::GetData(CompressedFieldsCache& cache) const
{
  Data res;
  int snamIndex = 0;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "SNAM", 4) && snamIndex < 3) {
        res.wordOfPower[snamIndex] = *reinterpret_cast<const uint32_t*>(data);
        res.spell[snamIndex] = *reinterpret_cast<const uint32_t*>(data + 4);
        res.recoveryTime[snamIndex] =
          *reinterpret_cast<const float*>(data + 8);
        snamIndex++;
      }
    },
    cache);
  return res;
}

}
