#pragma once
#include "Promise.h"
#include <chrono>

namespace Viet {

class Timer
{
public:
  Timer();

  template <typename T>
  Promise<Void> SetTimer(T&& duration, uint32_t* outId)
  {
    auto endTime = std::chrono::system_clock::now() + duration;
    return Set(endTime, outId);
  };

  [[maybe_unused]] bool RemoveTimer(uint32_t timerId);
  void TickTimers();

private:
  Promise<Void> Set(const std::chrono::system_clock::time_point& endTime,
                    uint32_t* outId);

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};

}
