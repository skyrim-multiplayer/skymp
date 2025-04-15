#include "RespawnPercentagesBinding.h"
#include "NapiHelper.h"

Napi::Value RespawnPercentagesBinding::Get(Napi::Env env,
                                           ScampServer& scampServer,
                                           uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  auto percentages = Napi::Object::New(env);
  percentages.Set("health",
                  Napi::Number::New(env, actor.GetHealthRespawnPercentage()));
  percentages.Set("magicka",
                  Napi::Number::New(env, actor.GetMagickaRespawnPercentage()));
  percentages.Set("stamina",
                  Napi::Number::New(env, actor.GetStaminaRespawnPercentage()));
  return percentages;
}

void RespawnPercentagesBinding::Set(Napi::Env env, ScampServer& scampServer,
                                    uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto newPercentages = NapiHelper::ExtractObject(newValue, "newPercentages");

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  float healthPercentage = NapiHelper::ExtractFloat(
    newPercentages.Get("health"), "newPercentages.health");
  float magickaPercentage = NapiHelper::ExtractFloat(
    newPercentages.Get("magicka"), "newPercentages.magicka");
  float staminaPercentage = NapiHelper::ExtractFloat(
    newPercentages.Get("stamina"), "newPercentages.stamina");

  actor.SetHealthRespawnPercentage(healthPercentage);
  actor.SetMagickaRespawnPercentage(magickaPercentage);
  actor.SetStaminaRespawnPercentage(staminaPercentage);
}
