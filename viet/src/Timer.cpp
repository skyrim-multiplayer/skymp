#include "Timer.h"
#include <algorithm>
#include <chrono>
#include <deque>
#include <set>
#include <utility>

#include <iostream>

namespace {
struct TimerEntry
{
  Viet::Promise<Viet::Void> promise;
  std::chrono::system_clock::time_point finish;
};
}

struct Viet::Timer::Impl
{
  std::deque<TimerEntry> timers;
};

Viet::Timer::Timer()
{
  pImpl = std::make_shared<Impl>();
}

Viet::Promise<Viet::Void> Viet::Timer::SetTimer(
  const std::chrono::system_clock::time_point& endTime)
{
  return Set(endTime);
}

void Viet::Timer::TickTimers()
{
  auto now = std::chrono::system_clock::now();

  auto& timers = pImpl->timers;
  while (!timers.empty() && now >= timers.front().finish) {
    auto front = std::move(timers.front());
    timers.pop_front();
    front.promise.Resolve(Viet::Void());
  }
}

bool Viet::Timer::RemoveTimer(
  const std::chrono::system_clock::time_point& endTime)
{
  auto& timers = pImpl->timers;
  auto it =
    std::lower_bound(timers.begin(), timers.end(), endTime,
                     [](const TimerEntry& entry,
                        const std::chrono::system_clock::time_point& target) {
                       return entry.finish < target;
                     });
  if (it != timers.end()) {
    timers.erase(it);
    return true;
  }
  return false;
}

Viet::Promise<Viet::Void> Viet::Timer::Set(
  const std::chrono::system_clock::time_point& endTime)
{
  Viet::Promise<Viet::Void> promise;
  bool sortRequired =
    !pImpl->timers.empty() && endTime > pImpl->timers.front().finish;

  pImpl->timers.push_front({ promise, endTime });

  if (sortRequired) {
    std::sort(pImpl->timers.begin(), pImpl->timers.end(),
              [](const TimerEntry& lhs, const TimerEntry& rhs) {
                return lhs.finish < rhs.finish;
              });
  }
  return promise;
}
