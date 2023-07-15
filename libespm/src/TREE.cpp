#include "libespm/TREE.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

TREE::Data TREE::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "EDID", 4))
        result.editorId = data;
      else if (!std::memcmp(type, "FULL", 4))
        result.fullName = data;
      else if (!std::memcmp(type, "OBND", 4))
        result.bounds = reinterpret_cast<const ObjectBounds*>(data);
      else if (!std::memcmp(type, "PFIG", 4))
        result.resultItem = *reinterpret_cast<const uint32_t*>(data);
      else if (!std::memcmp(type, "SNAM", 4))
        result.useSound = *reinterpret_cast<const uint32_t*>(data);
    },
    compressedFieldsCache);
  return result;
}

}
