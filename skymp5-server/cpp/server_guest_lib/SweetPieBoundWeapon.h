#pragma once
#include <array>
#include <chrono>
#include <cstdint>

class SweetPieBoundWeapon
{
public:
  enum class SkillLevel : uint8_t
  {
    Novice = 0,
    Adept,
    Expert,
    Master
  };
  SweetPieBoundWeapon(uint32_t baseId_, SkillLevel skillLevel_);
  float GetCooldown() const noexcept;
  float GetManacost() const noexcept;
  float GetManacostPercentage() const noexcept;
  uint32_t GetBaseId() const noexcept;

private:
  uint32_t baseId;
  SkillLevel skillLevel;
  const static std::array<float, 4> kCooldowns;
  const static std::array<float, 4> kManacostPercentages;
};
