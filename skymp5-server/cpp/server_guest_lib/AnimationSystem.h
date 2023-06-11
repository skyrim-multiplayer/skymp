#pragma once
#include "papyrus-vm/CIString.h"
#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

class MpActor;
struct AnimationData;

class AnimationSystem
{
public:
  AnimationSystem(bool isSweetpie);
  void Process(MpActor* actor, const AnimationData& animData);
  void ClearInfo(MpActor* actor);

private:
  using AnimationCallback = std::function<void(MpActor*)>;
  using AnimationCallbacks = CIMap<AnimationCallback>;
  using AnimationTimePoints =
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point>;

  void InitAnimationCallbacks(bool isSweetpie);
  std::chrono::steady_clock::time_point GetLastAttackReleaseAnimationTime(
    MpActor* actor) const;
  void SetLastAttackReleaseAnimationTime(
    MpActor* actor,
    std::chrono::steady_clock::time_point timePoint =
      std::chrono::steady_clock::now());

  AnimationCallbacks animationCallbacks;
  AnimationTimePoints lastAttackReleaseAnimationTimePoints;
};
