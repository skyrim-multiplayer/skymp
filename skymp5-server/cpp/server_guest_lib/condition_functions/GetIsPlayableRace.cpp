#include "GetIsPlayableRace.h"
#include "MpActor.h"
#include "WorldState.h"
#include "libespm/RACE.h"
#include <spdlog/spdlog.h>

const char* ConditionFunctions::GetIsPlayableRace::GetName() const
{
  return "GetIsPlayableRace";
}

uint16_t ConditionFunctions::GetIsPlayableRace::GetFunctionIndex() const
{
  return 254;
}

float ConditionFunctions::GetIsPlayableRace::Execute(
  MpActor& actor, [[maybe_unused]] uint32_t parameter1,
  [[maybe_unused]] uint32_t parameter2, const ConditionEvaluatorContext&)
{
  const uint32_t raceId = actor.GetRaceId();

  WorldState* worldState = actor.GetParent();
  if (!worldState) {
    return 0.f;
  }

  auto& espm = worldState->GetEspm();
  auto lookupResult = espm.GetBrowser().LookupById(raceId);
  if (!lookupResult.rec) {
    spdlog::error("ConditionFunctions::GetIsPlayableRace - race record not "
                  "found for id {:#x}",
                  raceId);
    return 0.f;
  }

  if (lookupResult.rec->GetType() != espm::RACE::kType) {
    spdlog::error("ConditionFunctions::GetIsPlayableRace - record {:#x} is "
                  "not a RACE (type {})",
                  raceId, lookupResult.rec->GetType().ToString());
    return 0.f;
  }

  {
    auto race = static_cast<const espm::RACE*>(lookupResult.rec);
    auto data = race->GetData(worldState->GetEspmCache());
    if (data.flags & espm::RACE::kPlayable) {
      return 1.0f;
    }
  }
  return 0.f;
}
