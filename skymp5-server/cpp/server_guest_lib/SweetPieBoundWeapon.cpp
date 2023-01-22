#include "SweetPieBoundWeapon.h"

const std::array<std::chrono::minutes, 4> SweetPieBoundWeapon::kCooldowns = {
  std::chrono::minutes(3),
  std::chrono::minutes(4),
  std::chrono::minutes(5),
  std::chrono::minutes(6),
};

const std::array<float, 4> SweetPieBoundWeapon::kManacosts = {
  30.f,
  40.f,
  50.f,
  60.f,
};

SweetPieBoundWeapon::SweetPieBoundWeapon(uint32_t baseId_,
                                         SkillLevel skillLevel_)
  : baseId(baseId_)
  , skillLevel(skillLevel_)
{
}

std::chrono::minutes SweetPieBoundWeapon::GetCooldown() const
{
  return kCooldowns[skillLevel];
}

float SweetPieBoundWeapon::GetPercentageManacost() const
{
  return kPercentageManacosts[skillLevel];
}

uint32_t SweetPieBoundWeapon::GetBaseId() const
{
  return baseId;
}
