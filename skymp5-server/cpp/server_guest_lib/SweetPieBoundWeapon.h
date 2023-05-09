#pragma once
#include <array>
#include <chrono>
#include <cstdint>

class SweetPieBoundWeapon
{
public:
  enum SkillLevel : uint8_t
  {
    Novice = 0,
    Adept,
    Expert,
    Master
  };
  SweetPieBoundWeapon(uint32_t baseId_, SkillLevel skillLevel_);
  float GetCooldown() const;
  float GetManacost() const;
  float GetManacostPercentage() const;
  uint32_t GetBaseId() const;

private:
  uint32_t baseId;
  SkillLevel skillLevel;
  const static std::array<float, 4> kCooldowns;
  const static std::array<float, 4> kManacostPercentages;
};
