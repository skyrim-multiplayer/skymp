#include "AnimationSystem.h"
#include "AnimationData.h"
#include "MpActor.h"

AnimationSystem::AnimationSystem()
{
  animations = { "blockStart", "blockStop", "attackStart" };
}

void AnimationSystem::Process(MpActor* const actor,
                              const AnimationData& animData) const
{
  if (!animations.count(animData.animEventName)) {
    return;
  }
  if (!strcmp(animData.animEventName, "blockStart")) {
    actor->SetIsBlockActive(true);
  } else if (!strcmp(animData.animEventName, "blockStop")) {
    actor->SetIsBlockActive(false);
  } else if (!strcmp(animData.animEventName, "attackStart")) {
    constexpr float modifier = 5.f;
    actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
  }
}
