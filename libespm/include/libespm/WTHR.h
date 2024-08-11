#pragma once
#include "RecordHeader.h"
#include <string>
#include <vector>

#pragma pack(push, 1)

namespace espm {

class WTHR final : public RecordHeader
{
public:
  static constexpr auto kType = "WTHR";

  struct Data
  {
    std::string editorId;
    std::vector<std::string> cloudTextures;
    uint32_t numberOfTextureLayers;
    uint32_t precipitation;
    uint32_t visualEffect;
    std::vector<float> cloudSpeeds;
    std::vector<std::array<float, 4>> cloudTextureColors;
    std::vector<std::array<float, 4>> cloudTextureAlphas;
    std::vector<std::array<float, 4>> colorDefinitions;
    std::array<float, 8> fogDistances;
    std::array<uint8_t, 19> weatherData;
    std::vector<std::pair<uint32_t, uint32_t>> ambientSounds;
    uint32_t skyStatics;
    std::array<uint32_t, 4> imageSpaces;
    std::vector<std::array<float, 6>> directionalAmbient;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(WTHR) == sizeof(RecordHeader));

}

#pragma pack(pop)
