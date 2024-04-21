#include "libespm/SCRL.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

SCRL::Data SCRL::GetData(CompressedFieldsCache& cache) const
{
  Data res;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        res.data.weight = *reinterpret_cast<const float*>(data + 0x4);
      }
    },
    cache);
  return res;
}

}
