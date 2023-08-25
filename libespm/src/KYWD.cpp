#include "libespm/KYWD.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

KYWD::Data KYWD::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "EDID", 4)) {
        result.editorId = data;
      }
    },
    compressedFieldsCache);
  return result;
}

}
