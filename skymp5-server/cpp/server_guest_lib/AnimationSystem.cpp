#include "AnimationSystem.h"
#include "ActiveMagicEffectsMap.h"
#include "AnimationData.h"
#include "MathUtils.h"
#include "MpActor.h"
#include "libespm/espm.h"
#include <unordered_map>

AnimationSystem::AnimationSystem(bool isSweetpie, WorldState& worldState_)
  : worldState(worldState_)
{
  InitAnimationCallbacks(isSweetpie);
}

std::unordered_map<std::string_view, float>
  AnimationSystem::s_weaponStaminaModifiers;

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
      [this](MpActor* actor) {
        constexpr float defaultModifier = 7.f;
        HandleAttackAnim(actor, defaultModifier);
      },
    },
    {
      "attackStartLeftHand",
      [this](MpActor* actor) {
        constexpr float defaultModifier = 7.f;
        HandleAttackAnim(actor, defaultModifier);
      },
    },
    {
      "AttackStartH2HRight",
      [this](MpActor* actor) {
        constexpr float defaultModifier = 4.f;
        HandleAttackAnim(actor, defaultModifier);
      },
    },
    {
      "AttackStartH2HLeft",
      [this](MpActor* actor) {
        constexpr float defaultModifier = 4.f;
        HandleAttackAnim(actor, defaultModifier);
      },
    },
    {
      "JumpStandingStart",
      [](MpActor* actor) {
        constexpr float defaultModifier = 10.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "JumpDirectionalStart",
      [](MpActor* actor) {
        constexpr float defaultModifier = 15.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
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
          constexpr float defaultModifier = 20.f;
          HandleAttackAnim(actor, defaultModifier);
        } else {
          constexpr float defaultModifier = 60.f;
          actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
        }
      },
    },
    {
      "crossbowAttackStart",
      [this](MpActor* actor) {
        constexpr float defaultModifier = 10.f;
        HandleAttackAnim(actor, defaultModifier);
      },
    },
    {
      "SneakSprintStartRoll",
      [](MpActor* actor) {
        constexpr float defaultModifier = 15.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackStartDualWield",
      [this](MpActor* actor) {
        constexpr float defaultModifier = 14.f;
        HandleAttackAnim(actor, defaultModifier);
      },
    },
    {
      "attackPowerStartInPlace",
      [](MpActor* actor) {
        constexpr float defaultModifier = 30.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackPowerStartBackward",
      [](MpActor* actor) {
        constexpr float defaultModifier = 30.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackPowerStartLeft",
      [](MpActor* actor) {
        constexpr float defaultModifier = 30.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackPowerStartRight",
      [](MpActor* actor) {
        constexpr float defaultModifier = 30.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackPowerStartDualWield",
      [](MpActor* actor) {
        constexpr float defaultModifier = 30.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackPowerStartForward",
      [](MpActor* actor) {
        constexpr float defaultModifier = 30.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackPowerStart_2HWSprint",
      [](MpActor* actor) {
        constexpr float defaultModifier = 30.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackPowerStart_2HWSprint",
      [](MpActor* actor) {
        constexpr float defaultModifier = 30.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackStartSprint",
      [](MpActor* actor) {
        constexpr float defaultModifier = 15.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },
    {
      "attackPowerStart_2HMSprint",
      [](MpActor* actor) {
        constexpr float defaultModifier = 30.f;
        actor->DamageActorValue(espm::ActorValue::Stamina, defaultModifier);
      },
    },

  };

  if (isSweetpie) {
    animationCallbacks.insert(additionalCallbacks.begin(),
                              additionalCallbacks.end());
  }
}

std::vector<uint32_t> AnimationSystem::GetWeaponKeywordFormIds(
  uint32_t baseId) const
{
  auto& espmBrowser = worldState.GetEspm().GetBrowser();
  return espmBrowser.LookupById(baseId).rec->GetKeywordIds(
    worldState.GetEspmCache());
}

std::vector<std::string_view> AnimationSystem::GetWeaponKeywords(
  uint32_t baseId) const
{
  std::vector<std::string_view> keywords;
  std::vector<uint32_t> keywordFormIds = GetWeaponKeywordFormIds(baseId);
  keywords.reserve(keywordFormIds.size());
  for (auto formId : keywordFormIds) {
    auto data = espm::GetData<espm::KYWD>(formId, &worldState);
    keywords.push_back(std::move(data.editorId));
  }
  return keywords;
}

float AnimationSystem::ComputeWeaponStaminaModifier(uint32_t baseId) const
{
  for (auto keyword : GetWeaponKeywords(baseId)) {
    if (auto it = s_weaponStaminaModifiers.find(keyword);
        it != s_weaponStaminaModifiers.end()) {
      return it->second;
    }
  }
  return 0.f;
}

void AnimationSystem::HandleAttackAnim(MpActor* actor,
                                       float defaultModifier) const
{
  float modifier = 0.f;
  std::apply(
    [&](const auto&... element) {
      ((modifier += element.has_value()
          ? ComputeWeaponStaminaModifier(element.value())
          : 0.f),
       ...);
    },
    actor->GetEquippedWeapon());
  modifier =
    MathUtils::IsNearlyEqual(0.f, modifier) ? defaultModifier : modifier;
  actor->DamageActorValue(espm::ActorValue::Stamina, modifier);
}

void AnimationSystem::SetWeaponStaminaModifiers(
  std::unordered_map<std::string_view, float>&& modifiers)
{
  s_weaponStaminaModifiers = modifiers;
}
