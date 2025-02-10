#include "DamageMultFormula.h"

#include "MpActor.h"
#include <nlohmann/json.hpp>

namespace {

bool IsNonPlayerBaseId(const MpActor& actor)
{
  return actor.GetBaseId() != 0x7;
}

DamageMultFormula::Settings ParseConfig(const nlohmann::json& config)
{
  DamageMultFormula::Settings settings;

  if (!config.is_object()) {
    spdlog::warn(
      "Invalid damage mult formula config format. Using default one.");
    return settings;
  }

  if (!config.contains("multiplier")) {
    spdlog::warn("Unable to get multiplier from config. Using default damage "
                 "mult formula settings");
    return settings;
  }

  settings.multiplier = config.at("multiplier").get<float>();

  return settings;
}

}

DamageMultFormula::DamageMultFormula(
  std::unique_ptr<IDamageFormula> baseFormula_, const nlohmann::json& config_)
  : baseFormula(std::move(baseFormula_))
{
  settings = ParseConfig(config_);
}

float DamageMultFormula::CalculateDamage(const MpActor& aggressor,
                                         const MpActor& target,
                                         const HitData& hitData) const
{
  float baseDamage = baseFormula->CalculateDamage(aggressor, target, hitData);

  auto worldState = aggressor.GetParent();
  if (!worldState) {
    return baseDamage;
  }

  if (IsNonPlayerBaseId(aggressor) && !IsNonPlayerBaseId(target)) {
    baseDamage *= settings.multiplier;
  }

  return baseDamage;
}

float DamageMultFormula::CalculateDamage(
  const MpActor& aggressor, const MpActor& target,
  const SpellCastData& spellCastData) const
{
  float baseDamage =
    baseFormula->CalculateDamage(aggressor, target, spellCastData);

  auto worldState = aggressor.GetParent();
  if (!worldState) {
    return baseDamage;
  }

  if (IsNonPlayerBaseId(aggressor) && !IsNonPlayerBaseId(target)) {
    baseDamage *= settings.multiplier;
  }

  return baseDamage;
}
