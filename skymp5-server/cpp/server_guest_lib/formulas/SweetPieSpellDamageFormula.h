#pragma once

#include "IDamageFormula.h"
#include "MpActor.h"

#include <memory>
#include <optional>
#include <vector>

#include <nlohmann/json_fwd.hpp>

// Modifies vanilla damage for spells

struct SweetPieSpellDamageFormulaSettingsEntry
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("itemId", itemId).Serialize("mult", mult);
  }

  std::string itemId; // supports hex and decimal ids: "0x000feef1", "0"
  float mult = 0.f;
};

struct SweetPieSpellDamageFormulaSettings
{

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("entries", entries);
  }

  static SweetPieSpellDamageFormulaSettings FromJson(const nlohmann::json& j);

  std::vector<SweetPieSpellDamageFormulaSettingsEntry> entries;
};

class SweetPieSpellDamageFormula : public IDamageFormula
{
public:
  SweetPieSpellDamageFormula(std::unique_ptr<IDamageFormula> baseFormula_,
                             const nlohmann::json& config);

  [[nodiscard]] float CalculateDamage(const MpActor& aggressor,
                                      const MpActor& target,
                                      const HitData& hitData) const override;

  [[nodiscard]] float CalculateDamage(
    const MpActor& aggressor, const MpActor& target,
    const SpellCastData& spellCastData) const override;

private:
  SweetPieSpellDamageFormulaSettings ParseConfig(
    const nlohmann::json& config) const;

private:
  std::unique_ptr<IDamageFormula> baseFormula;
  std::optional<SweetPieSpellDamageFormulaSettings> settings;
};
