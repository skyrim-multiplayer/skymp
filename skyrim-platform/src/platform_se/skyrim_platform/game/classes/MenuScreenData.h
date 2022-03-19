#pragma once

namespace RE {

class MenuScreenData
{
public:
  static MenuScreenData* GetSingleton()
  {
    REL::Relocation<MenuScreenData**> singleton{
      Offsets::MenuScreenData::Singleton
    };
    return *singleton;
  }

  std::uint32_t unk00;    // 00
  RE::NiPoint2 mousePos;  // 04
  float unk0C;            // 0C
  float unk10;            // 10
  float screenWidth;      // 14
  float screenHeight;     // 18
  float mouseSensitivity; // 1C
  float unk20;            // 20
  float unk24;            // 24
  float unk28;            // 28 - scaling related
  std::uint32_t unk2C;    // 2C
};
static_assert(sizeof(MenuScreenData) == 0x30);

}
