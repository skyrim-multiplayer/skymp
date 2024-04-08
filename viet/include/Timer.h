#pragma once
#include "Promise.h"
#include <chrono>

namespace Viet {

class Timer
{
public:
  Timer();

  template <typename T>
  Promise<Void> SetTimer(T&& duration, uint32_t* outTimerId)
  {
    auto endTime = std::chrono::system_clock::now() + duration;
    return Set(endTime, outTimerId);
  };

  [[maybe_unused]] bool RemoveTimer(uint32_t timerId);
  void TickTimers();

private:
  Promise<Void> Set(const std::chrono::system_clock::time_point& endTime,
                    uint32_t* outTimerId);

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};

}
