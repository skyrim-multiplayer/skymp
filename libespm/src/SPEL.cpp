#include "libespm/SPEL.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

SPEL::Data SPEL::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "SPIT", 4)) {
        result.type = *reinterpret_cast<const SpellType*>(data + 8);
      }
    },
    compressedFieldsCache);
  return result;
}

}
