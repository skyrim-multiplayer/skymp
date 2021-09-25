#pragma once
#include "CallNative.h"

class SendAnimationEvent
{
public:
  static void Run(const std::vector<CallNative::AnySafe>& _args);
};
