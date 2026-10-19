#include "ActorValuesBinding.h"
#include "MathUtils.h"
#include "NapiHelper.h"
#include <cmath>

namespace {
void ApplyActorValueIfPresent(const Napi::Object& newActorValues,
                              const char* key,
                              const char* debugPath,
                              espm::ActorValue av,
                              float oldValue,
                              MpActor& actor)
{
  if (!newActorValues.Has(key)) {
    return;
  }

  float value =
    NapiHelper::ExtractFloat(newActorValues.Get(key), debugPath);
  if (!std::isfinite(value)) {
    spdlog::warn("ActorValuesBinding::Set - {} must be finite", debugPath);
    return;
  }

  if (!MathUtils::IsNearlyEqual(oldValue, value)) {
    actor.SetActorValue(av, value);
  }
}
}

Napi::Value ActorValuesBinding::Get(Napi::Env env, ScampServer& scampServer,
                                    uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  auto& actorValues = actor.GetActorValues();

  auto result = Napi::Object::New(env);
  result.Set("health", Napi::Number::New(env, actorValues.health));
  result.Set("magicka", Napi::Number::New(env, actorValues.magicka));
  result.Set("stamina", Napi::Number::New(env, actorValues.stamina));
  result.Set("healRate", Napi::Number::New(env, actorValues.healRate));
  result.Set("magickaRate", Napi::Number::New(env, actorValues.magickaRate));
  result.Set("staminaRate", Napi::Number::New(env, actorValues.staminaRate));
  result.Set("healRateMult", Napi::Number::New(env, actorValues.healRateMult));
  result.Set("magickaRateMult",
             Napi::Number::New(env, actorValues.magickaRateMult));
  result.Set("staminaRateMult",
             Napi::Number::New(env, actorValues.staminaRateMult));
  return result;
}

void ActorValuesBinding::Set(Napi::Env env, ScampServer& scampServer,
                             uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();
  auto newActorValues = NapiHelper::ExtractObject(newValue, "newActorValues");

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  const ActorValues oldActorValues = actor.GetActorValues();

  ApplyActorValueIfPresent(newActorValues, "health", "newActorValues.health",
                           espm::ActorValue::Health, oldActorValues.health,
                           actor);
  ApplyActorValueIfPresent(newActorValues, "magicka",
                           "newActorValues.magicka",
                           espm::ActorValue::Magicka, oldActorValues.magicka,
                           actor);
  ApplyActorValueIfPresent(newActorValues, "stamina",
                           "newActorValues.stamina",
                           espm::ActorValue::Stamina, oldActorValues.stamina,
                           actor);
  ApplyActorValueIfPresent(newActorValues, "healRate",
                           "newActorValues.healRate",
                           espm::ActorValue::HealRate, oldActorValues.healRate,
                           actor);
  ApplyActorValueIfPresent(
    newActorValues, "magickaRate", "newActorValues.magickaRate",
    espm::ActorValue::MagickaRate, oldActorValues.magickaRate, actor);
  ApplyActorValueIfPresent(
    newActorValues, "staminaRate", "newActorValues.staminaRate",
    espm::ActorValue::StaminaRate, oldActorValues.staminaRate, actor);
  ApplyActorValueIfPresent(
    newActorValues, "healRateMult", "newActorValues.healRateMult",
    espm::ActorValue::HealRateMult_or_CombatHealthRegenMultMod,
    oldActorValues.healRateMult, actor);
  ApplyActorValueIfPresent(
    newActorValues, "magickaRateMult", "newActorValues.magickaRateMult",
    espm::ActorValue::MagickaRateMult_or_CombatHealthRegenMultPowerMod,
    oldActorValues.magickaRateMult, actor);
  ApplyActorValueIfPresent(
    newActorValues, "staminaRateMult", "newActorValues.staminaRateMult",
    espm::ActorValue::StaminaRateMult, oldActorValues.staminaRateMult, actor);
}
