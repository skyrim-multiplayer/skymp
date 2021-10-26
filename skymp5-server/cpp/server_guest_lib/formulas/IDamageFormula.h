#pragma once

class IDamageFormula
{
public:
  ~IDamageFormula() = default;

  virtual float CalculateDamage() const = 0;
};
