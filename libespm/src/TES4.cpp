#include "libespm/TES4.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

TES4::Data TES4::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "HEDR", 4))
        result.header = reinterpret_cast<const Header*>(data);
      else if (!std::memcmp(type, "CNAM", 4))
        result.author = data;
      else if (!std::memcmp(type, "SNAM", 4))
        result.description = data;
      else if (!std::memcmp(type, "MAST", 4))
        result.masters.push_back(data);
    },
    compressedFieldsCache);
  return result;
}

}
