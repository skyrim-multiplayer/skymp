#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <string>

class MpActor;
struct AnimationData;

class AnimationSystem
{
public:
  AnimationSystem();
  void Process(MpActor* actor, const AnimationData& animData) const;

private:
  using AnimationCallback = std::function<void(MpActor*)>;
  using AnimationCallbacks = std::unordered_map<std::string, AnimationCallback>;

  void InitAnimationCallbacks();
  void AddAnimationCallback(std::string animEvenName, AnimationCallback callback);

  AnimationCallbacks animationCallbacks;
};
