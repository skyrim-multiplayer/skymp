#pragma once
#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

class MpActor;
struct AnimationData;

class AnimationSystem
{
public:
  AnimationSystem();
  void Process(MpActor* actor, const AnimationData& animData);

private:
  using AnimationCallback = std::function<void(MpActor*)>;
  using AnimationCallbacks =
    std::unordered_map<std::string, AnimationCallback>;

  void InitAnimationCallbacks();
  std::chrono::steady_clock::time_point GetLastAttackReleaseAnimationTime()
    const;
  void SetLastAttackReleaseAnimationTime(
    std::chrono::steady_clock::time_point timePoint =
      std::chrono::steady_clock::now());

  AnimationCallbacks animationCallbacks;
  std::chrono::steady_clock::time_point lastAttackReleaseAnimationTime;
};
