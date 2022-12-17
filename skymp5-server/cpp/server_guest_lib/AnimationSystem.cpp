#include "AnimationSystem.h"
#include "AnimationData.h"
#include "MpActor.h"

AnimationSystem::AnimationSystem()
{
  InitAnimationCallbacks();
}

void AnimationSystem::Process(MpActor* actor,
                              const AnimationData& animData) const
{
  auto it = animationCallbacks.find(animData.animEventName);
  if (it == animationCallbacks.end()) {
    return;
  }
  it->second(actor);
}

void AnimationSystem::InitAnimationCallbacks()
{
  animationCallbacks = {
    {
      "blockStart",
      [&](MpActor* actor) { actor->SetIsBlockActive(true); },
    },
    { "blockStop", [&](MpActor* actor) { actor->SetIsBlockActive(false); } },
    { "attackStart",
      [&](MpActor* actor) {
        constexpr float modifier = 5.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      } },
    { "jumpStart",
      [&](MpActor* actor) {
        constexpr float modifier = 5.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      } }
  };
}
