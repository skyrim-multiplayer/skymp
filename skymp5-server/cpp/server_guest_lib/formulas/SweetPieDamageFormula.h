#pragma once

#include "IDamageFormula.h"
#include "MpActor.h"

#include <memory>
#include <unordered_map>

#include <nlohmann/json_fwd.hpp>

// Modifies vanilla damage

struct SweetPieDamageFormulaSettings
{
  std::vector<float> damageMultByLevel{};
  std::unordered_map<std::string, std::vector<uint32_t>> weaponKeywords{};
};

class SweetPieDamageFormula : public IDamageFormula
{
public:
  SweetPieDamageFormula(std::unique_ptr<IDamageFormula> baseFormula_,
                        const nlohmann::json& config);

  float CalculateDamage(const MpActor& aggressor, const MpActor& target,
                        const HitData& hitData) const override;

private:
  SweetPieDamageFormulaSettings ParseConfig(
    const nlohmann::json& config) const;

private:
  std::unique_ptr<IDamageFormula> baseFormula;
  SweetPieDamageFormulaSettings settings;
};
