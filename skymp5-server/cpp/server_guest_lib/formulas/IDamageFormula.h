#pragma once

class MpActor;
struct HitData;

class IDamageFormula
{
public:
  virtual ~IDamageFormula() = default;

  virtual float CalculateDamage(const MpActor& aggressor,
                                const MpActor& target,
                                const HitData& hitData) const = 0;
};
