#include "AnimationSystem.h"
#include "AnimationData.h"
#include "MpActor.h"
#include "espm.h"

AnimationSystem::AnimationSystem()
{
  InitAnimationCallbacks();
}

void AnimationSystem::Process(MpActor* actor, const AnimationData& animData)
{
  auto it = animationCallbacks.find(animData.animEventName);
  if (it == animationCallbacks.end()) {
    return;
  }
  it->second(actor);
}

std::chrono::steady_clock::time_point
AnimationSystem::GetLastAttackReleaseAnimationTime() const
{
  return lastAttackReleaseAnimationTime;
}

void AnimationSystem::SetLastAttackReleaseAnimationTime(
  std::chrono::steady_clock::time_point timePoint)
{
  lastAttackReleaseAnimationTime = timePoint;
}

using namespace std::chrono_literals;

void AnimationSystem::InitAnimationCallbacks()
{
  animationCallbacks = {
    {
      "blockStart",
      [](MpActor* actor) {
        constexpr float newRate = 0.f;
        actor->SetIsBlockActive(true);
        actor->SetActorValue(espm::ActorValue::StaminaRate, newRate);
      },
    },
    {
      "blockStop",
      [&](MpActor* actor) {
        actor->SetIsBlockActive(false);
        actor->SetActorValue(espm::ActorValue::StaminaRate,
                             actor->GetBaseValues().staminaRate);
      },
    },
    {
      "attackStart",
      [](MpActor* actor) {
        constexpr float modifier = 7.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      },
    },
    {
      "AttackStartH2HRight",
      [](MpActor* actor) {
        constexpr float modifier = 4.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      },
    },
    {
      "AttackStartH2HLeft",
      [](MpActor* actor) {
        constexpr float modifier = 4.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      },
    },
    {
      "JumpStandingStart",
      [](MpActor* actor) {
        constexpr float modifier = 10.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      },
    },
    {
      "JumpDirectionalStart",
      [](MpActor* actor) {
        constexpr float modifier = 15.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      },
    },
    {
      "bowAttackStart",
      [this](MpActor* actor) { SetLastAttackReleaseAnimationTime(); },
    },
    {
      "attackRelease",
      [&](MpActor* actor) {
        std::chrono::duration<float> elapsedTime =
          std::chrono::steady_clock::now() -
          GetLastAttackReleaseAnimationTime();
        if (elapsedTime > 2s) {
          constexpr float modifier = 20.f;
          actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
        } else {
          constexpr float modifier = 60.f;
          actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
        }
      },
    },
    {
      "crossbowAttackStart",
      [](MpActor* actor) {
        constexpr float modifier = 10.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      },
    },
    {
      "SneakSprintStartRoll",
      [](MpActor* actor) {
        constexpr float modifier = 15.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      },
    },
  };
}
