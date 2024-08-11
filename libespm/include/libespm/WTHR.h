#pragma once

#include <string>
#include <vector>
#include <array>

struct WTHR
{
  std::string editorId;
  std::vector<std::string> cloudTextures;
  uint32_t unkownLNAM;
  uint32_t precipitation;
  uint32_t visualEffect;
  std::array<float, 32> unkownRNAM;
  std::array<float, 32> unkownQNAM;
  std::array<std::array<float, 4>, 32> cloudTextureColors;
  std::array<std::array<float, 4>, 32> cloudTextureAlphas;
  std::array<std::array<float, 4>, 17> unknownNAM0;
  struct FogDistance
  {
    float dayNear;
    float dayFar;
    float nightNear;
    float nightFar;
    float dayPow;
    float nightPow;
    float dayMax;
    float nightMax;
  };
  struct Data
  {
    uint8_t windSpeed;
    uint8_t unknown1;
    uint8_t unknown2;
    uint8_t transDelta;
    uint8_t sunGlare;
    uint8_t sunDamage;
    uint8_t precipBeginFadeIn;
    uint8_t precipEndFadeOut;
    uint8_t thuderBeginFadeIn;
    uint8_t thunderEndFadeOut;
    uint8_t thunderFrequency;
    uint8_t flags;
    std::array<uint8_t, 3> precipLightingColor;
    uint8_t unknown3;
    uint8_t windDirection;
    uint8_t windDirRange;
  };
  uint32_t unknownNAM1;
  struct AmbientSound
  {
    uint32_t soundReference;
    uint32_t type;
  };
  std::vector<AmbientSound> ambientSounds;
  uint32_t skyStatics;
  std::array<uint32_t, 4> imageSpaces;
  struct DirectionalAmbient
  {
    std::array<float, 3> directionalAmbientXPlus;
    std::array<float, 3> directionalAmbientXMinus;
    std::array<float, 3> directionalAmbientYPlus;
    std::array<float, 3> directionalAmbientYMinus;
    std::array<float, 3> directionalAmbientZPlus;
    std::array<float, 3> directionalAmbientZMinus;
    std::array<float, 3> specularColor;
    float fresnelPower;
  } directionalAmbient;
  std::string auroraModel;
  std::array<uint32_t, 4> unknownNAM2;
  std::array<uint32_t, 4> unknownNAM3;
};

