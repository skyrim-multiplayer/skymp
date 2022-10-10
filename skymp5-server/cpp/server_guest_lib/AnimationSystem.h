#pragma once
#include <string>
#include <unordered_set>

class AnimationData;
class MpActor;

class AnimationSystem
{
public:
  AnimationSystem();
  void Process(MpActor* const actor, const AnimationData& animData) const;

private:
  std::unordered_set<std::string> animations;
};
