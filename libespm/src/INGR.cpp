#include "libespm/INGR.h"
#include "libespm/IterateFields.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

INGR::Data INGR::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  result.effects = Effects(this).GetData(compressedFieldsCache).effects;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        result.itemData.value = *reinterpret_cast<const uint32_t*>(data);
        result.itemData.weight = *reinterpret_cast<const float*>(data + 0x4);
      }
    },
    compressedFieldsCache);
  return result;
}

}
