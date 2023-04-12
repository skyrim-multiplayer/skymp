#include "PercentagesBinding.h"
#include "NapiHelper.h"

Napi::Value PercentagesBinding::Get(Napi::Env env, ScampServer& scampServer,
                                    uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  auto actorValues = actor.GetChangeForm().actorValues;
  auto percentages = Napi::Object::New(env);
  percentages.Set("health",
                  Napi::Number::New(env, actorValues.healthPercentage));
  percentages.Set("magicka",
                  Napi::Number::New(env, actorValues.magickaPercentage));
  percentages.Set("stamina",
                  Napi::Number::New(env, actorValues.staminaPercentage));
  return percentages;
}

void PercentagesBinding::Set(Napi::Env env, ScampServer& scampServer,
                             uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto newPercentages = NapiHelper::ExtractObject(newValue, "newPercentages");

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  ActorValues actorValues = actor.GetChangeForm().actorValues;
  actorValues.healthPercentage = NapiHelper::ExtractFloat(
    newPercentages.Get("health"), "newPercentages.health");
  actorValues.magickaPercentage = NapiHelper::ExtractFloat(
    newPercentages.Get("magicka"), "newPercentages.magicka");
  actorValues.staminaPercentage = NapiHelper::ExtractFloat(
    newPercentages.Get("stamina"), "newPercentages.stamina");
  actor.NetSetPercentages(actorValues);
}
