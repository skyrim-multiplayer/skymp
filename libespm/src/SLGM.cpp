#include "libespm/SLGM.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

SLGM::Data SLGM::GetData(CompressedFieldsCache& cache) const
{
  Data res;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "SOUL", 4)) {
        res.currentSoul = *reinterpret_cast<const uint8_t*>(data);
      } else if (!std::memcmp(type, "DATA", 4)) {
        res.baseValue = *reinterpret_cast<const uint32_t*>(data);
        res.weight = *reinterpret_cast<const float*>(data + 4);
      } else if (!std::memcmp(type, "SLCP", 4)) {
        res.soulCapacity = *reinterpret_cast<const uint8_t*>(data);
      }
    },
    cache);
  return res;
}

}
