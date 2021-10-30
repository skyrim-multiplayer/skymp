#pragma once

class IDamageFormula
{
public:
  virtual ~IDamageFormula() = default;

  virtual float CalculateDamage() const = 0;
};
