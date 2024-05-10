#include "libespm/BOOK.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

bool BOOK::Data::IsFlagSet(const Flags flag) const noexcept
{
  return (flags & flag) == flag;
}

BOOK::Data BOOK::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;

  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        result.flags =
          static_cast<Flags>(*reinterpret_cast<const uint8_t*>(data));

        result.spellOrSkillFormId =
          *reinterpret_cast<const uint32_t*>(data + 0x4);
        result.weight = *reinterpret_cast<const float*>(data + 0xc);
      }
    },
    compressedFieldsCache);

  return result;
}

}
