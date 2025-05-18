#include "GetIsRace.h"
#include "MpActor.h"

const char* ConditionFunctions::GetIsRace::GetName() const
{
  return "GetIsRace";
}

uint16_t ConditionFunctions::GetIsRace::GetFunctionIndex() const
{
  return 69;
}

float ConditionFunctions::GetIsRace::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2)
{
  if (actor.GetRaceId() == parameter1) {
    return 1.0f;
  }
  return 0.f;
}
