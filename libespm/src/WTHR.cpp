#include "libespm/WTHR.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

WTHR::Data WTHR::GetData(CompressedFieldsCache& cache) const
{
  Data res;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "EDID", 4)) {
        res.editorId = std::string(data, size);
      } else if (type[0] == '0' && type[1] == 'T' && type[2] == 'X') {
        res.cloudTextures.emplace_back(data, size);
      } else if (!std::memcmp(type, "LNAM", 4)) {
        res.numberOfTextureLayers = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "MNAM", 4)) {
        res.precipitation = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "NNAM", 4)) {
        res.visualEffect = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "RNAM", 4)) {
        for (int i = 0; i < 32; i += 4) {
          res.cloudSpeeds.push_back(*reinterpret_cast<const float*>(data + i));
        }
      } else if (!std::memcmp(type, "PNAM", 4)) {
        for (int i = 0; i < 512; i += 16) {
          std::array<float, 4> colors;
          for (int j = 0; j < 4; ++j) {
            colors[j] = *reinterpret_cast<const float*>(data + i + j * 4);
          }
          res.cloudTextureColors.push_back(colors);
        }
      } else if (!std::memcmp(type, "JNAM", 4)) {
        for (int i = 0; i < 512; i += 16) {
          std::array<float, 4> alphas;
          for (int j = 0; j < 4; ++j) {
            alphas[j] = *reinterpret_cast<const float*>(data + i + j * 4);
          }
          res.cloudTextureAlphas.push_back(alphas);
        }
      } else if (!std::memcmp(type, "NAM0", 4)) {
        for (int i = 0; i < size; i += 16) {
          std::array<float, 4> colors;
          for (int j = 0; j < 4; ++j) {
            colors[j] = *reinterpret_cast<const float*>(data + i + j * 4);
          }
          res.colorDefinitions.push_back(colors);
        }
      } else if (!std::memcmp(type, "FNAM", 4)) {
        for (int i = 0; i < 32; i += 4) {
          res.fogDistances[i / 4] = *reinterpret_cast<const float*>(data + i);
        }
      } else if (!std::memcmp(type, "DATA", 4)) {
        std::memcpy(res.weatherData.data(), data, 19);
      } else if (!std::memcmp(type, "SNAM", 4)) {
        res.ambientSounds.emplace_back(
          *reinterpret_cast<const uint32_t*>(data),
          *reinterpret_cast<const uint32_t*>(data + 4));
      } else if (!std::memcmp(type, "TNAM", 4)) {
        res.skyStatics = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "IMSP", 4)) {
        for (int i = 0; i < 16; i += 4) {
          res.imageSpaces[i / 4] =
            *reinterpret_cast<const uint32_t*>(data + i);
        }
      } else if (!std::memcmp(type, "DALC", 4)) {
        for (int i = 0; i < size; i += 24) {
          std::array<float, 6> ambient;
          for (int j = 0; j < 6; ++j) {
            ambient[j] = *reinterpret_cast<const float*>(data + i + j * 4);
          }
          res.directionalAmbient.push_back(ambient);
        }
      }
    },
    cache);
  return res;
}

}
