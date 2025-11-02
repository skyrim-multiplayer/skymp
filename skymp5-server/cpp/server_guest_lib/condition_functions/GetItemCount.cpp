#include "GetItemCount.h"
#include "MpActor.h"

const char* ConditionFunctions::GetItemCount::GetName() const
{
  return "GetItemCount";
}

uint16_t ConditionFunctions::GetItemCount::GetFunctionIndex() const
{
  return 47;
}

float ConditionFunctions::GetItemCount::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext&)
{
  return static_cast<float>(actor.GetInventory().GetItemCount(parameter1));
}
