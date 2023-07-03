#pragma once
#include "Promise.h"
#include <chrono>

namespace Viet {
class Timer
{
public:
  Timer();

  template <typename T>
  Viet::Promise<Viet::Void> SetTimer(T&& duration)
  {
    auto endTime = std::chrono::system_clock::now() + duration;
    return Set(endTime);
  };

  Viet::Promise<Viet::Void> SetTimer(
    const std::chrono::system_clock::time_point& endTime);
  bool RemoveTimer(const std::chrono::system_clock::time_point& endTime);
  void TickTimers();

private:
  Viet::Promise<Viet::Void> Set(
    const std::chrono::system_clock::time_point& endTime);

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
}
