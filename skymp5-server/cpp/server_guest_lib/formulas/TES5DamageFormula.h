#pragma once

#include "IDamageFormula.h"

#include <spdlog/spdlog.h>

#include "MpActor.h"
#include "HitData.h"
#include "WorldState.h"
#include <espm.h>

class TES5DamageFormula
{
public:
  TES5DamageFormula(const MpActor& aggressor_, const MpActor& target_,
                const HitData& hitData_);

  float GetBaseWeaponDamage() const;
  float CalcWeaponRating() const;
  float CalcArmorRatingComponent(const Inventory::Entry& opponentEquipmentEntry) const;
  float CalcOpponentArmorRating() const;

  float CalculateDamage() const;

private:
  const MpActor& aggressor;
  const MpActor& target;
  const HitData& hitData;
  WorldState* espmProvider;
};
