#include "SpellHasKeyword.h"
#include "MpActor.h"

const char* ConditionFunctions::SpellHasKeyword::GetName() const
{
  return "SpellHasKeyword";
}

uint16_t ConditionFunctions::SpellHasKeyword::GetFunctionIndex() const
{
  return 596;
}

float ConditionFunctions::SpellHasKeyword::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2)
{
  return 0.0f; // TODO
}
