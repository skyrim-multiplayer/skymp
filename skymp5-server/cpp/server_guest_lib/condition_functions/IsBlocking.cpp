#include "IsBlocking.h"
#include "MpActor.h"

const char* ConditionFunctions::IsBlocking::GetName() const
{
  return "IsBlocking";
}

uint16_t ConditionFunctions::IsBlocking::GetFunctionIndex() const
{
  return 569;
}

float ConditionFunctions::IsBlocking::Execute(
  MpActor& actor, [[maybe_unused]] uint32_t parameter1,
  [[maybe_unused]] uint32_t parameter2, const ConditionEvaluatorContext&)
{
  return actor.GetAnimationVariableBool("IsBlocking") ? 1.0f : 0.0f;
}
