#include "libespm/ALCH.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

ALCH::Data ALCH::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  result.effects = Effects(this).GetData(compressedFieldsCache).effects;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        result.weight = *reinterpret_cast<const float*>(data);
      }
    },
    compressedFieldsCache);
  return result;
}

}
