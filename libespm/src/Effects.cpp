#include "libespm/Effects.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>
#include <fmt/format.h>

namespace espm {

Effects::Effects(const RecordHeader* parent)
  : parent(parent)
{
}

Effects::Data Effects::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  if (!parent) {
    return Data();
  }

  Data result;
  uint32_t effectIndex = 0;
  bool orderFlag = true;
  bool isValid = true;

  RecordHeaderAccess::IterateFields(
    parent,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "EFID", 4)) {
        isValid = orderFlag == true;
        result.effects.emplace_back();
        result.effects[effectIndex].effectId =
          *reinterpret_cast<const uint32_t*>(data);
        orderFlag = false;
      } else if (!std::memcmp(type, "EFIT", 4)) {
        isValid = orderFlag == false;
        Effect& eff = result.effects[effectIndex];
        eff.magnitude = *reinterpret_cast<const float*>(data);
        eff.areaOfEffect = *reinterpret_cast<const uint32_t*>(data + 4);
        eff.duration = *reinterpret_cast<const uint32_t*>(data + 8);
        effectIndex++;
        orderFlag = true;
      }
      if (!isValid) {
        auto name = parent->GetEditorId(compressedFieldsCache);
        throw std::runtime_error(
          fmt::format("Bad effect array for edid={}", name));
      }
    },
    compressedFieldsCache);
  return result;
}

}
