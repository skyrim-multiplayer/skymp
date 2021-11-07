#pragma once
#include "Promise.h"

namespace Viet {
class Timer
{
public:
  Timer();

  Viet::Promise<Viet::Void> SetTimer(float seconds);
  void TickTimers();

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
}
