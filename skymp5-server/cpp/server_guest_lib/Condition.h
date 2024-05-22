#pragma once
#include "MpActor.h"
#include <spdlog/spdlog.h>

class Condition
{
protected:
  espm::CTDA::Operator conditionOperator;
  espm::CTDA::Flags flags;

public:
  Condition(espm::CTDA::Operator conditionOperator, espm::CTDA::Flags flags);
  virtual bool Evaluate(MpActor* actor) const = 0;
  virtual std::string GetDescription() const = 0;
  espm::CTDA::Flags GetFlags() const;
  virtual ~Condition() = default;
};

class ItemCountCondition : public Condition
{
  int itemId = 0;
  int comparisonValue = 0;

public:
  ItemCountCondition(int itemId, int comparisonValue,
                     espm::CTDA::Operator conditionOperator,
                     espm::CTDA::Flags flags);

  bool Evaluate(MpActor* actor) const override;

  std::string GetDescription() const override;
};

class RaceCondition : public Condition
{
  int raceId = 0;
  int comparisonValue = 0;

public:
  RaceCondition(int raceId, int comparisonValue,
                espm::CTDA::Operator conditionOperator,
                espm::CTDA::Flags flags);

  bool Evaluate(MpActor* actor) const override;

  std::string GetDescription() const override;
};
