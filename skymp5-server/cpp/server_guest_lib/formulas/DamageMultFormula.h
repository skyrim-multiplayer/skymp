#pragma once
#include <memory>

#include "IDamageFormula.h"
#include <nlohmann/json_fwd.hpp>

class DamageMultFormula : public IDamageFormula
{
public:
  DamageMultFormula(std::unique_ptr<IDamageFormula> baseFormula_,
                    const nlohmann::json& config);

  float CalculateDamage(const MpActor& aggressor, const MpActor& target,
                        const HitData& hitData) const override;

public:
  struct Settings
  {
    float multiplier = 1.f;
  };

private:
  std::unique_ptr<IDamageFormula> baseFormula;
  Settings settings;
};
