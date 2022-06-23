#pragma once

/**
 * This class was copied from skse
 * --
 * for some reason in commonlib TESRace has TintAsset member
 * which looks to be exactly like TintMask
 * but TESNPC and PlayerCharacter both have TintMask member
 * which is a stub class with no definition
 * most likely needs fixing on their side
 */
class TintMask
{
public:
  TintMask()
  {
    alpha = 0.0;
    tintType = 0;
    texture = nullptr;
  };
  ~TintMask(){};

#ifdef PAPYRUS_CUSTOM_CLASS
  enum
  {
    kTypeID = 300
  };
#endif

  enum
  {
    kMaskType_Freckles = 0,
    kMaskType_Lips,
    kMaskType_Cheeks,
    kMaskType_Eyeliner,
    kMaskType_UpperEyeSocket,
    kMaskType_LowerEyeSocket,
    kMaskType_SkinTone,
    kMaskType_WarPaint,
    kMaskType_FrownLines,
    kMaskType_LowerCheeks,
    kMaskType_Nose,
    kMaskType_Chin,
    kMaskType_Neck,
    kMaskType_Forehead,
    kMaskType_Dirt
  };

  RE::TESTexture* texture;

  union
  {
    struct Color
    {
      uint8_t red, green, blue,
        alpha; // The alpha isn't actually used here so its usually zero
    } color;
    uint32_t abgr;
  };

  float alpha;
  uint32_t tintType;

  uint32_t ToARGB()
  {
    return ((((uint32_t)(alpha * 255) & 0xFF) << 24) |
            ((color.red & 0xFF) << 16) | ((color.green & 0xFF) << 8) |
            ((color.blue & 0xFF) << 0));
  }
};
