#pragma once
#include "AnimationData.h"
#include "MpActor.h"

class AnimationSystem
{
public:
  AnimationSystem()
  {
    animations = { "blockStart", "blockStop", "attackStart" };
  }
  void Process(MpActor* actor, const AnimationData& animData)
  {
    if (!animations.count(animData.animEventName)) {
      return;
    }
    if (!strcmp(animData.animEventName, "blockStart")) {
      actor->SetIsBlockActive(true);
    } else if (!strcmp(animData.animEventName, "blockStop")) {
      actor->SetIsBlockActive(false);
    } else if (!strcmp(animData.animEventName, "attackStart")) {
      actor->DamageActorValue(espm::ActorValue::Stamina, 5.f);
    }
  }

private:
  std::unordered_set<std::string> animations;
};