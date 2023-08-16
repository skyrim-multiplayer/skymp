#include "libespm/GMST.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

GMST::Data GMST::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        result.value = *reinterpret_cast<const float*>(data);
      }
    },
    compressedFieldsCache);
  return result;
}

}
