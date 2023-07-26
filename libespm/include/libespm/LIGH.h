#pragma once
#include "libespm/CompressedFieldsCache.h"
#include "libespm/RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class LIGH final : public RecordHeader
{
public:
  static constexpr auto kType = "LIGH";

  enum Flags : uint32_t
  {
    Dynamic = 0x0001,
    CanBeCarired = 0x0002,
    EffectFlicker = 0x0008,
    OffByDefault = 0x00020,
    EffectFlickerSlow = 0x0040,
    EffectPulse = 0x0080,
    TypeShadowSpotlight = 0x0400,
    TypeShadowHemisphere = 0x0800,
    TypeShadowOmnidirectional = 0x1000,
    PortalStrict = 0x2000
  };

  struct DATA
  {
    int32_t time;
    uint32_t radius;
    uint32_t color;
    uint32_t flags;
    float falloffExponent;
    float fov;
    float nearClip;
    float frequency; // 1/period on uesp.net
    float intensityAmplitude;
    float movementAmplitude;
    uint32_t value;
    float weight;
  };
  static_assert(sizeof(DATA) == 0x30);

  struct Data
  {
    DATA data;
  };

  Data GetData(CompressedFieldsCache& cache) const noexcept;
};

static_assert(sizeof(LIGH) == sizeof(RecordHeader));

}

#pragma pack(pop)
