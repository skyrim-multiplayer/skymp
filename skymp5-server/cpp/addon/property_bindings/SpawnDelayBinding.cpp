#include "SpawnDelayBinding.h"
#include "NapiHelper.h"

Napi::Value SpawnDelayBinding::Get(Napi::Env env, ScampServer& scampServer,
                                   uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  return Napi::Number::New(env, actor.GetRespawnTime());
}

void SpawnDelayBinding::Set(Napi::Env env, ScampServer& scampServer,
                            uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto newSpawnDelay = NapiHelper::ExtractFloat(newValue, "newSpawnDelay");

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  actor.SetRespawnTime(newSpawnDelay);
}
