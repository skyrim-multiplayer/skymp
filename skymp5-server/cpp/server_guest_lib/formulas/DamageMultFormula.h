#pragma once
#include <memory>

#include "IDamageFormula.h"

class DamageMultFormula : public IDamageFormula
{
public:
  DamageMultFormula(std::unique_ptr<IDamageFormula> baseFormula_);

  float CalculateDamage(const MpActor& aggressor, const MpActor& target,
                        const HitData& hitData) const override;

private:
  std::unique_ptr<IDamageFormula> baseFormula;
};
