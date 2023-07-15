#include "libespm/MGEF.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>
#include <type_traits>

namespace espm {

MGEF::Data MGEF::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        result.data.type = EffectType{
          *reinterpret_cast<const std::underlying_type_t<EffectType>*>(data +
                                                                       0x40)
        };
        result.data.primaryAV = ActorValue(
          *reinterpret_cast<const std::underlying_type_t<ActorValue>*>(data +
                                                                       0x44));
      }
    },
    compressedFieldsCache);

  return result;
}

}
