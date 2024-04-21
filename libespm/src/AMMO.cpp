#include "libespm/AMMO.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

AMMO::Data AMMO::GetData(CompressedFieldsCache& cache) const
{
  Data res;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        // NOTE: 0x10 offset is for SSE version only
        res.weight = *reinterpret_cast<const float*>(data + 0x10);
      }
    },
    cache);
  return res;
}

}
