#pragma once
#include "papyrus-vm/CIString.h"
#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

class MpActor;
class WorldState;
struct AnimationData;

class AnimationSystem
{
public:
  AnimationSystem();
  void Init(WorldState* worldState);
  void Process(MpActor* actor, const AnimationData& animData);
  void ClearInfo(MpActor* actor);
  void SetWeaponStaminaModifiers(
    std::unordered_map<std::string, float>&& modifiers);

private:
  using AnimationCallback = std::function<void(MpActor*)>;
  using AnimationCallbacks = CIMap<AnimationCallback>;
  using AnimationTimePoints =
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point>;

  void InitAdditionalCallbacks();
  std::chrono::steady_clock::time_point GetLastAttackReleaseAnimationTime(
    MpActor* actor) const;
  void SetLastAttackReleaseAnimationTime(
    MpActor* actor,
    std::chrono::steady_clock::time_point timePoint =
      std::chrono::steady_clock::now());
  std::vector<uint32_t> GetWeaponKeywordFormIds(uint32_t baseId) const;
  std::vector<std::string_view> GetWeaponKeywords(uint32_t baseId) const;
  float ComputeWeaponStaminaModifier(uint32_t baseId) const;
  void HandleAttackAnim(MpActor* actor, float defaultModifier) const;
  bool IsWorldStateAttached() const noexcept;

private:
  std::unordered_map<std::string, float> weaponStaminaModifiers;

  AnimationCallbacks animationCallbacks;
  AnimationTimePoints lastAttackReleaseAnimationTimePoints;
  WorldState* worldState = nullptr;
  bool hasSweetpie = false;
};
