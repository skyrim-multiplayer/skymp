#include "PercentagesBinding.h"
#include "MathUtils.h"
#include "NapiHelper.h"
#include <cmath>

Napi::Value PercentagesBinding::Get(Napi::Env env, ScampServer& scampServer,
                                    uint32_t formId)
{
  float outHealthPercentageBeforeDeath = 0.f;
  float outMagickaPercentageBeforeDeath = 0.f;
  float outStaminaPercentageBeforeDeath = 0.f;

  if (scampServer.IsGameModeInsideDeathEventHandler(
        formId, &outHealthPercentageBeforeDeath,
        &outMagickaPercentageBeforeDeath, &outStaminaPercentageBeforeDeath)) {
    auto percentages = Napi::Object::New(env);
    percentages.Set("health",
                    Napi::Number::New(env, outHealthPercentageBeforeDeath));
    percentages.Set("magicka",
                    Napi::Number::New(env, outMagickaPercentageBeforeDeath));
    percentages.Set("stamina",
                    Napi::Number::New(env, outStaminaPercentageBeforeDeath));
    return percentages;
  }

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
  if (scampServer.IsGameModeInsideDeathEventHandler(formId)) {
    spdlog::warn("PercentagesBinding::Set - cannot set percentages during "
                 "death event of the same actor {:x}",
                 formId);
    return;
  }

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
      std::isnan(actorValues.healthPercentage)) {
    spdlog::warn(
      "PercentagesBinding::Set - healthPercentage must be in [0, 1]");
    if (actorValues.healthPercentage < 0) {
      actorValues.healthPercentage = 0.f;
    } else {
      actorValues.healthPercentage = 1.0f;
    }
  }

  if (actorValues.magickaPercentage < 0 || actorValues.magickaPercentage > 1 ||
      std::isnan(actorValues.magickaPercentage)) {
    spdlog::warn(
      "PercentagesBinding::Set - magickaPercentage must be in [0, 1]");
    if (actorValues.magickaPercentage < 0) {
      actorValues.magickaPercentage = 0.0f;
    } else {
      actorValues.magickaPercentage = 1.0f;
    }
  }

  if (actorValues.staminaPercentage < 0 || actorValues.staminaPercentage > 1 ||
      std::isnan(actorValues.staminaPercentage)) {
    spdlog::warn(
      "PercentagesBinding::Set - staminaPercentage must be in [0, 1]");
    if (actorValues.staminaPercentage < 0) {
      actorValues.staminaPercentage = 0.0f;
    } else {
      actorValues.staminaPercentage = 1.0f;
    }
  }

  const ActorValues oldActorValues = actor.GetActorValues();

  std::vector<espm::ActorValue> avFilter;

  if (!MathUtils::IsNearlyEqual(oldActorValues.healthPercentage,
                                actorValues.healthPercentage)) {
    avFilter.push_back(espm::ActorValue::Health);
  }
  if (!MathUtils::IsNearlyEqual(oldActorValues.magickaPercentage,
                                actorValues.magickaPercentage)) {
    avFilter.push_back(espm::ActorValue::Magicka);
  }
  if (!MathUtils::IsNearlyEqual(oldActorValues.staminaPercentage,
                                actorValues.staminaPercentage)) {
    avFilter.push_back(espm::ActorValue::Stamina);
  }

  actor.NetSetPercentages(actorValues, nullptr, avFilter);
}
