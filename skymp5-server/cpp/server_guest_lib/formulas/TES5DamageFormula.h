#pragma once

#include "IDamageFormula.h"

#include <spdlog/spdlog.h>

#include "MpActor.h"
#include "HitData.h"
#include "WorldState.h"
#include <espm.h>

// Implements vanilla Skyrim damage formula.
// Some parts may be missing. If they are, there should be a TODO regarding it.
// If there's no corresponding TODO, consider adding it and/or filing an issue.

class TES5DamageFormula : public IDamageFormula
{
public:
  TES5DamageFormula(const MpActor& aggressor_, const MpActor& target_,
                const HitData& hitData_);

  float CalculateDamage() const override;

private:
  const MpActor& aggressor;
  const MpActor& target;
  const HitData& hitData;
  WorldState* espmProvider;

private:
  float GetBaseWeaponDamage() const;
  float CalcWeaponRating() const;
  float CalcArmorRatingComponent(const Inventory::Entry& opponentEquipmentEntry) const;
  float CalcOpponentArmorRating() const;
};
