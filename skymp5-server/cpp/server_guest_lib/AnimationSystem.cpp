#include "AnimationSystem.h"
#include "AnimationData.h"
#include "MpActor.h"
#include "libespm/espm.h"

AnimationSystem::AnimationSystem(bool isSweetpie)
{
  InitAnimationCallbacks(isSweetpie);
}

void AnimationSystem::Process(MpActor* actor, const AnimationData& animData)
{
  auto it = animationCallbacks.find(animData.animEventName);
  if (it == animationCallbacks.end()) {
    return;
  }
  it->second(actor);
}

void AnimationSystem::ClearInfo(MpActor* actor)
{
  lastAttackReleaseAnimationTimePoints.erase(actor->GetFormId());
}

std::chrono::steady_clock::time_point
AnimationSystem::GetLastAttackReleaseAnimationTime(MpActor* actor) const
{
  auto it = lastAttackReleaseAnimationTimePoints.find(actor->GetFormId());
  if (it == lastAttackReleaseAnimationTimePoints.end()) {
    return std::chrono::steady_clock::time_point();
  }
  return it->second;
}

void AnimationSystem::SetLastAttackReleaseAnimationTime(
  MpActor* actor, std::chrono::steady_clock::time_point timePoint)
{
  lastAttackReleaseAnimationTimePoints[actor->GetFormId()] = timePoint;
}

void AnimationSystem::InitAnimationCallbacks(bool isSweetpie)
{
  animationCallbacks = {
    {
      "blockStart",
      [isSweetpie](MpActor* actor) {
        constexpr float newRate = 0.f;
        actor->SetIsBlockActive(true);
        if (isSweetpie) {
          actor->SetActorValue(espm::ActorValue::StaminaRate, newRate);
        }
      },
    },
    {
      "blockStop",
      [isSweetpie](MpActor* actor) {
        actor->SetIsBlockActive(false);
        if (isSweetpie) {
          actor->SetActorValue(espm::ActorValue::StaminaRate,
                               actor->GetBaseValues().staminaRate);
        }
      },
    }
  };
  const AnimationCallbacks additionalCallbacks = {
    {
      "attackStart",
      [](MpActor* actor) {
        constexpr float modifier = 7.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      },
    },
    {
      "attackStartLeftHand",
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
      [this](MpActor* actor) { SetLastAttackReleaseAnimationTime(actor); },
    },
    {
      "attackRelease",
      [this](MpActor* actor) {
        std::chrono::duration<float> elapsedTime =
          std::chrono::steady_clock::now() -
          GetLastAttackReleaseAnimationTime(actor);
        if (elapsedTime > std::chrono::seconds(2)) {
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
    {
      "attackStartDualWield",
      [](MpActor* actor) {
        constexpr float modifier = 14.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
      },
    }
  };

  if (isSweetpie) {
    animationCallbacks.insert(additionalCallbacks.begin(),
                              additionalCallbacks.end());
  }
}
