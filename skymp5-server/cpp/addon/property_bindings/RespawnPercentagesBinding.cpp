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

  if (healthPercentage <= 0 || healthPercentage > 1 ||
      isnan(healthPercentage)) {
    spdlog::warn(
      "RespawnPercentagesBinding::Set - healthPercentage must be in (0, 1]");
    if (healthPercentage <= 0) {
      healthPercentage = 0.01f;
    } else {
      healthPercentage = 1.0f;
    }
  }

  if (magickaPercentage < 0 || magickaPercentage > 1 ||
      isnan(magickaPercentage)) {
    spdlog::warn(
      "RespawnPercentagesBinding::Set - magickaPercentage must be in [0, 1]");
    if (magickaPercentage < 0) {
      magickaPercentage = 0.0f;
    } else {
      magickaPercentage = 1.0f;
    }
  }

  if (staminaPercentage < 0 || staminaPercentage > 1 ||
      isnan(staminaPercentage)) {
    spdlog::warn(
      "RespawnPercentagesBinding::Set - staminaPercentage must be in [0, 1]");
    if (staminaPercentage < 0) {
      staminaPercentage = 0.0f;
    } else {
      staminaPercentage = 1.0f;
    }
  }

  actor.SetHealthRespawnPercentage(healthPercentage);
  actor.SetMagickaRespawnPercentage(magickaPercentage);
  actor.SetStaminaRespawnPercentage(staminaPercentage);
}
