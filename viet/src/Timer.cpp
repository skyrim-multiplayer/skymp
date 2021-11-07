#include "Timer.h"
#include <algorithm>
#include <chrono>
#include <deque>

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

Viet::Promise<Viet::Void> Viet::Timer::SetTimer(float seconds)
{
  Viet::Promise<Viet::Void> promise;

  auto finish = std::chrono::system_clock::now() +
    std::chrono::milliseconds(static_cast<int>(seconds * 1000));

  bool sortRequired =
    !pImpl->timers.empty() && finish > pImpl->timers.front().finish;

  pImpl->timers.push_front({ promise, finish });

  if (sortRequired) {
    std::sort(pImpl->timers.begin(), pImpl->timers.end(),
              [](const TimerEntry& lhs, const TimerEntry& rhs) {
                return lhs.finish < rhs.finish;
              });
  }

  return promise;
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
