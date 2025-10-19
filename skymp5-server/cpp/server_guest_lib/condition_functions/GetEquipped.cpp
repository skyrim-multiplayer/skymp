#include "GetEquipped.h"
#include "MpActor.h"

const char* ConditionFunctions::GetEquipped::GetName() const
{
  return "GetEquipped";
}

uint16_t ConditionFunctions::GetEquipped::GetFunctionIndex() const
{
  return 182;
}

float ConditionFunctions::GetEquipped::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2, const ConditionEvaluatorContext&)
{
  if (actor.GetEquipment().inv.GetItemCount(parameter1) > 0) {
    return 1.f;
  }
  return 0.f;
}
