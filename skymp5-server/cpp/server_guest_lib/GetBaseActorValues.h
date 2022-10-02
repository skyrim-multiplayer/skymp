#pragma once
#include "ActorValues.h"
#include <Loader.h>
#include <WorldState.h>
#include <cstdint>
#include <espm.h>
#include <fmt/format.h>

struct BaseActorValues : public ActorValues
{
  using PropertiesVisitor =
    std::function<void(const char* propName, const char* jsonValue)>;

  void VisitBaseActorValues(BaseActorValues& baseActorValues,
                            MpChangeForm& changeForm,
                            const PropertiesVisitor& visitor);

  float GetValue(espm::ActorValue av);
};

BaseActorValues GetBaseActorValues(WorldState* worldState, uint32_t baseId,
                                   uint32_t raceIdOverride);
