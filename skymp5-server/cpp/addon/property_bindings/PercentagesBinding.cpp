#include "PercentagesBinding.h"
#include "NapiHelper.h"

Napi::Value PercentagesBinding::Get(Napi::Env env, ScampServer& scampServer,
                                    uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  auto& actorValues = actor.GetActorValues();
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

  ActorValues actorValues = actor.GetActorValues();

  actorValues.healthPercentage = NapiHelper::ExtractFloat(
    newPercentages.Get("health"), "newPercentages.health");
  actorValues.magickaPercentage = NapiHelper::ExtractFloat(
    newPercentages.Get("magicka"), "newPercentages.magicka");
  actorValues.staminaPercentage = NapiHelper::ExtractFloat(
    newPercentages.Get("stamina"), "newPercentages.stamina");

  if (actorValues.healthPercentage < 0 || actorValues.healthPercentage > 1 ||
      isnan(actorValues.healthPercentage)) {
    spdlog::warn(
      "PercentagesBinding::Set - healthPercentage must be in [0, 1]");
    if (actorValues.healthPercentage < 0) {
      actorValues.healthPercentage = 0.f;
    } else {
      actorValues.healthPercentage = 1.0f;
    }
  }

  if (actorValues.magickaPercentage < 0 || actorValues.magickaPercentage > 1 ||
      isnan(actorValues.magickaPercentage)) {
    spdlog::warn(
      "PercentagesBinding::Set - magickaPercentage must be in [0, 1]");
    if (actorValues.magickaPercentage < 0) {
      actorValues.magickaPercentage = 0.0f;
    } else {
      actorValues.magickaPercentage = 1.0f;
    }
  }

  if (actorValues.staminaPercentage < 0 || actorValues.staminaPercentage > 1 ||
      isnan(actorValues.staminaPercentage)) {
    spdlog::warn(
      "PercentagesBinding::Set - staminaPercentage must be in [0, 1]");
    if (actorValues.staminaPercentage < 0) {
      actorValues.staminaPercentage = 0.0f;
    } else {
      actorValues.staminaPercentage = 1.0f;
    }
  }

  actor.NetSetPercentages(actorValues);
}
