#include "libespm/OTFT.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

OTFT::Data OTFT::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "INAM", 4)) {
        result.formIds = reinterpret_cast<const uint32_t*>(data);
        result.count = dataSize / sizeof(dataSize);
      }
    },
    compressedFieldsCache);
  return result;
}

}
