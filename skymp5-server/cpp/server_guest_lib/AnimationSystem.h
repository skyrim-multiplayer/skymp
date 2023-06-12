#pragma once
#include "CIString.h"
#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

class MpActor;
class MpObjectReference;
struct AnimationData;

class AnimationSystem
{
public:
  AnimationSystem(bool isSweetpie);

  void Process(MpActor& objectReference, const AnimationData& animData);
  void ClearInfo(MpObjectReference& objectReference);

private:
  using AnimationCallback = std::function<void(MpActor*)>;
  using AnimationCallbacks = CIMap<AnimationCallback>;
  using AnimationTimePoints =
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point>;

private:
  void InitAnimationCallbacks(bool isSweetpie);
  std::chrono::steady_clock::time_point GetLastAttackReleaseAnimationTime(
    MpObjectReference& objectReference) const noexcept;
  void SetLastAttackReleaseAnimationTime(
    MpObjectReference& objectReference,
    std::chrono::steady_clock::time_point timePoint =
      std::chrono::steady_clock::now());

private:
  AnimationCallbacks animationCallbacks;
  AnimationTimePoints lastAttackReleaseAnimationTimePoints;
};
