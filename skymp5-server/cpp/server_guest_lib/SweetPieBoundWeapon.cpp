#include "SweetPieBoundWeapon.h"
#include <type_traits>

const std::array<float, 4> SweetPieBoundWeapon::kCooldowns = {
  180.f,
  240.f,
  300.f,
  360.f,
};

const std::array<float, 4> SweetPieBoundWeapon::kManacostPercentages = {
  0.3f,
  0.4f,
  0.5f,
  0.6f,
};

SweetPieBoundWeapon::SweetPieBoundWeapon(uint32_t baseId_,
                                         SkillLevel skillLevel_)
  : baseId(baseId_)
  , skillLevel(skillLevel_)
{
}

float SweetPieBoundWeapon::GetCooldown() const noexcept
{
  return kCooldowns[static_cast<std::underlying_type_t<SkillLevel>>(
    skillLevel)];
}

float SweetPieBoundWeapon::GetManacost() const noexcept
{
  return kManacostPercentages[static_cast<std::underlying_type_t<SkillLevel>>(
           skillLevel)] *
    100;
}

float SweetPieBoundWeapon::GetManacostPercentage() const noexcept
{
  return kManacostPercentages[static_cast<std::underlying_type_t<SkillLevel>>(
    skillLevel)];
}

uint32_t SweetPieBoundWeapon::GetBaseId() const noexcept
{
  return baseId;
}
