#include "GetItemCount.h"

const char* ConditionFunctions::GetItemCount::GetName() const
{
  return "GetItemCount";
}

uint16_t ConditionFunctions::GetItemCount::GetFunctionIndex() const
{
  return 47;
}

float ConditionFunctions::GetItemCount::Execute(MpActor& actor,
                                                uint32_t parameter1,
                                                uint32_t parameter2)
{
  return static_cast<float>(actor.GetInventory().GetItemCount(parameter1));
}
