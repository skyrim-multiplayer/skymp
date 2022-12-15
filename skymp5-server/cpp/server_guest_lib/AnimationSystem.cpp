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
  AddAnimationCallback("blockStart",
                       [&](MpActor* actor) { actor->SetIsBlockActive(true); });
  AddAnimationCallback(
    "blockStop", [&](MpActor* actor) { actor->SetIsBlockActive(false); });
  AddAnimationCallback("attackStart", [&](MpActor* actor) {
    constexpr float modifier = 5.f;
    actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
  });
  AddAnimationCallback("jumpStart", [&](MpActor* actor) {
    constexpr float modifier = 5.f;
    actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
  });
}

void AnimationSystem::AddAnimationCallback(std::string animEventName,
                                           AnimationCallback callback)
{
  animationCallbacks[animEventName] = callback;
}
