#include "TES5DamageFormula.h"

namespace {

bool IsUnarmedAttack(const uint32_t sourceFormId)
{
  return sourceFormId == 0x1f4;
}

}

TES5DamageFormula::TES5DamageFormula(const MpActor& aggressor_,
                                     const MpActor& target_,
                                     const HitData& hitData_)
  : aggressor(aggressor_)
  , target(target_)
  , hitData(hitData_)
  , espmProvider(aggressor.GetParent())
{
}

float TES5DamageFormula::GetBaseWeaponDamage() const
{
  auto weapData = espm::GetData<espm::WEAP>(hitData.source, espmProvider);
  if (!weapData.weapData) {
    throw std::runtime_error(
      fmt::format("no weapData for {:#x}", hitData.source));
  }
  return weapData.weapData->damage;
}

float TES5DamageFormula::CalcWeaponRating() const
{
  // TODO(#xyz): take other components into account
  return GetBaseWeaponDamage();
}

float TES5DamageFormula::CalcArmorRatingComponent(
  const Inventory::Entry& opponentEquipmentEntry) const
{
  spdlog::info("XXX type for {:#x} is {}", opponentEquipmentEntry.baseId,
               espm::GetRecordType(opponentEquipmentEntry.baseId, espmProvider)
                 .ToString());

  if (opponentEquipmentEntry.extra.worn != Inventory::Worn::None &&
      espm::GetRecordType(opponentEquipmentEntry.baseId, espmProvider) ==
        espm::ARMO::type) {
    auto armorData =
      espm::GetData<espm::ARMO>(opponentEquipmentEntry.baseId, espmProvider);
    spdlog::info("armor baseId={:#x}: baseValue={}",
                  opponentEquipmentEntry.baseId, armorData.baseValue);
    // TODO(#xyz): take other components into account
    return armorData.baseValue;
  }
  return 0;
}

float TES5DamageFormula::CalcOpponentArmorRating() const
{
  // TODO(#xyz): OpponentArmorRating is 1 if your character is successfully
  // sneaking and has the Master Sneak perk (C) UESP Wiki
  float combinedArmorRating = 0;
  for (const auto& entry : target.GetEquipment().inv.entries) {
    spdlog::info("CalculateDamage {} -> {}; item '{}', baseId={:#x}; worn={}",
                 aggressor.idx, target.idx, entry.extra.name, entry.baseId,
                 static_cast<int>(entry.extra.worn));
    combinedArmorRating += CalcArmorRatingComponent(entry);
  }
  // combinedArmorRating = std::min(combinedArmorRating, 85.0f);
  // return (100 - combinedArmorRating) / 100;
  return combinedArmorRating;
}

float TES5DamageFormula::CalculateDamage() const
{
  if (IsUnarmedAttack(hitData.source)) {
    uint32_t raceId = aggressor.GetRaceId();
    return espm::GetData<espm::RACE>(raceId, espmProvider).unarmedDamage;
  }

  float incomingDamage = CalcWeaponRating();
  float maxArmorRating = espm::GetData<espm::GMST>(espm::GMST::kFArmorRating, espmProvider).value;
  float minReceivedDamage = incomingDamage * (1 - 0.01 * maxArmorRating);

  // TODO(#xyz): take other components into account
  // return CalcWeaponRating() * CalcOpponentArmorRating();
  float damage = std::max<float>(minReceivedDamage, incomingDamage / (CalcOpponentArmorRating() * 0.12 + 1));
  if (hitData.isPowerAttack) {
    damage *= 2;
  }
  if (hitData.isHitBlocked) {
    // TODO(#xyz): implement correct block formula
    damage *= 0.1;
  }
  return damage;
}
