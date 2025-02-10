#pragma once
#include <memory>

#include "IDamageFormula.h"
#include <nlohmann/json_fwd.hpp>

class DamageMultFormula : public IDamageFormula
{
public:
  DamageMultFormula(std::unique_ptr<IDamageFormula> baseFormula_,
                    const nlohmann::json& config);

  [[nodiscard]] float CalculateDamage(const MpActor& aggressor,
                                      const MpActor& target,
                                      const HitData& hitData) const override;

  [[nodiscard]] float CalculateDamage(
    const MpActor& aggressor, const MpActor& target,
    const SpellCastData& spellCastData) const override;

public:
  struct Settings
  {
    float multiplier = 2.f;
  };

private:
  std::unique_ptr<IDamageFormula> baseFormula;
  Settings settings;
};
