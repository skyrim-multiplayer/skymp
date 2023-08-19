#include "libespm/WRLD.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

WRLD::Data WRLD::GetData(CompressedFieldsCache& cache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        result.flags = *reinterpret_cast<const uint8_t*>(data);
      }
    },
    cache);
  return result;
}

}
