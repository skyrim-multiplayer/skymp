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
        result.spellItem = reinterpret_cast<const SPITData*>(data);
      } else if (!std::memcmp(type, "EFID", 4)) {
        result.effects.emplace_back(*reinterpret_cast<const uint32_t*>(data));
      } else if (!std::memcmp(type, "EFIT", 4)) {
        result.effects.back().effectItem = reinterpret_cast<const EFIT*>(data);
      }
    },
    compressedFieldsCache);
  return result;
}

}
