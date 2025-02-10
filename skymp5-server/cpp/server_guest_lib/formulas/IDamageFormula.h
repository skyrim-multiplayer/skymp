#pragma once

class MpActor;
struct HitData;
struct SpellCastData;

class IDamageFormula
{
public:
  virtual ~IDamageFormula() = default;

  [[nodiscard]] virtual float CalculateDamage(
    const MpActor& aggressor, const MpActor& target,
    const HitData& hitData) const = 0;

  [[nodiscard]] virtual float CalculateDamage(
    const MpActor& aggressor, const MpActor& target,
    const SpellCastData& spellCastData) const = 0;
};
