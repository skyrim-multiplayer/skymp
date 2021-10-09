#pragma once
#include <Loader.h>
#include <WorldState.h>
#include <cstdint>
#include <espm.h>
#include <fmt/format.h>

struct BaseActorValues
{
  float health = 100.f;
  float stamina = 100.f;
  float magicka = 100.f;
  float healRate = 0.7f;
  float staminaRate = 5.0f;
  float magickaRate = 3.0f;
  float healRateMult = 100.f;
  float staminaRateMult = 100.f;
  float magickaRateMult = 100.f;

  using PropertiesVisitor =
    std::function<void(const char* propName, const char* jsonValue)>;

  void VisitBaseActorValues(BaseActorValues& baseActorValues,
                            MpChangeForm& changeForm,
                            const PropertiesVisitor& visitor);
};

BaseActorValues GetBaseActorValues(espm::Loader& espm, uint32_t baseId,
                                   uint32_t raceIdOverride);
