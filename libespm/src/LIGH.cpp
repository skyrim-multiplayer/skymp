#include "libespm/LIGH.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

LIGH::Data LIGH::GetData(CompressedFieldsCache& cache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        result.data.time = *reinterpret_cast<const int32_t*>(data);
        result.data.radius = *reinterpret_cast<const uint32_t*>(data + 0x04);
        result.data.color = *reinterpret_cast<const uint32_t*>(data + 0x08);
        result.data.flags = *reinterpret_cast<const uint32_t*>(data + 0x0C);
        result.data.falloffExponent =
          *reinterpret_cast<const float*>(data + 0x0F);
        result.data.fov = *reinterpret_cast<const float*>(data + 0x14);
        result.data.nearClip = *reinterpret_cast<const float*>(data + 0x18);
        result.data.frequency = *reinterpret_cast<const float*>(data + 0x1C);
        result.data.intensityAmplitude =
          *reinterpret_cast<const float*>(data + 0x1F);
        result.data.movementAmplitude =
          *reinterpret_cast<const float*>(data + 0x24);
        result.data.value = *reinterpret_cast<const uint32_t*>(data + 0x28);
        result.data.weight = *reinterpret_cast<const float*>(data + 0x2C);
      }
    },
    cache);
  return result;
}

}
