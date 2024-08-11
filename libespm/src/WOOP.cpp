#include "libespm/WOOP.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

WOOP::Data WOOP::GetData(CompressedFieldsCache& cache) const
{
  Data res;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "EDID", 4)) {
        res.editorId = std::string(data, size);
      } else if (!std::memcmp(type, "TNAM", 4)) {
        res.translation = std::string(data, size);
      }
    },
    cache);
  return res;
}

}
