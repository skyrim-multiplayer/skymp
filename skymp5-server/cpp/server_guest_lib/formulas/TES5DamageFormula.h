#pragma once

#include "IDamageFormula.h"

#include <spdlog/spdlog.h>

#include "MpActor.h"
#include "HitData.h"
#include "WorldState.h"
#include <espm.h>

bool IsUnarmedAttack(const uint32_t sourceFormId)
{
  return sourceFormId == 0x1f4;
}

uint32_t GetRaceId(const MpActor& actor)
{
  auto appearance = actor.GetAppearance();
  if (appearance) {
    return appearance->raceId;
  }
  WorldState* espmProvider = actor.GetParent();
  uint32_t baseId = actor.GetBaseId();
  return espm::GetData<espm::NPC_>(baseId, espmProvider).race;
}

class DamageFormula
{
public:
  DamageFormula(const MpActor& aggressor_, const MpActor& target_,
                const HitData& hitData_)
    : aggressor(aggressor_)
    , target(target_)
    , hitData(hitData_)
    , espmProvider(aggressor.GetParent())
  {
  }

  float GetBaseWeaponDamage() const
  {
    auto weapData = espm::GetData<espm::WEAP>(hitData.source, espmProvider);
    if (!weapData.weapData) {
      throw std::runtime_error(fmt::format("no weapData for {:#x}", hitData.source));
    }
    return weapData.weapData->damage;
  }

  float CalcWeaponRating() const
  {
    // TODO(#xyz): take other components into account
    return GetBaseWeaponDamage();
  }

  float CalcArmorRatingComponent(const Inventory::Entry& opponentEquipmentEntry) const
  {
    spdlog::info("XXX type for {:#x} is {}", opponentEquipmentEntry.baseId, espm::GetRecordType(opponentEquipmentEntry.baseId, espmProvider).ToString());

    if (opponentEquipmentEntry.extra.worn != Inventory::Worn::None && espm::GetRecordType(opponentEquipmentEntry.baseId, espmProvider) == espm::ARMO::type) {
      try {
        auto armorData =
          espm::GetData<espm::ARMO>(opponentEquipmentEntry.baseId, espmProvider);
        spdlog::info("armor baseId={:#x}: baseValue={}", opponentEquipmentEntry.baseId,
                      armorData.baseValue);
        // TODO(#xyz): take other components into account
        return armorData.baseValue;
      } catch (const std::exception& exc) {
        // XXX: gotta check if that's ARMO somehow. New GetData for already found rec?
        spdlog::error("err: {}", exc.what());
      }
    }
    return 0;
  }

  float CalcOpponentArmorRating() const
  {
    // TODO(#xyz): OpponentArmorRating is 1 if your character is successfully sneaking and has the Master Sneak perk
    // (C) UESP Wiki
    float combinedArmorRating = 0;
    for (const auto& entry : target.GetEquipment().inv.entries) {
      spdlog::info("CalculateDamage {} -> {}; item '{}', baseId={:#x}; worn={}",
                   aggressor.idx, target.idx, entry.extra.name, entry.baseId,
                   static_cast<int>(entry.extra.worn));
      combinedArmorRating += CalcArmorRatingComponent(entry);
    }
    combinedArmorRating = std::min(combinedArmorRating, 85.0f);
    return (100 - combinedArmorRating) / 100;
  }

  float CalculateDamage() const
  {
    if (IsUnarmedAttack(hitData.source)) {
      uint32_t raceId = GetRaceId(aggressor);
      return espm::GetData<espm::RACE>(raceId, espmProvider).unarmedDamage;
    }

    // TODO(#xyz): take other components into account
    return CalcWeaponRating() * CalcOpponentArmorRating();
  }

private:
  const MpActor& aggressor;
  const MpActor& target;
  const HitData& hitData;
  WorldState* espmProvider;
};
