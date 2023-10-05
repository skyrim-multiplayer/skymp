#include "Timer.h"
#include <MakeID.h>
#include <algorithm>
#include <chrono>
#include <deque>
#include <limits>
#include <memory>
#include <spdlog/spdlog.h>
#include <utility>

namespace Viet {

namespace {
struct TimerEntry
{
  uint32_t id;
  Promise<Void> promise;
  std::chrono::system_clock::time_point finish;
};
}

struct Timer::Impl
{
  std::deque<TimerEntry> timers;
  const std::unique_ptr<MakeID> idGenerator =
    std::make_unique<MakeID>(std::numeric_limits<uint32_t>::max());

  void DestroyID(const TimerEntry& entry);
};

void Timer::Impl::DestroyID(const TimerEntry& entry)
{
  idGenerator->DestroyID(entry.id);
}

Timer::Timer()
{
  pImpl = std::make_shared<Impl>();
}

void Timer::TickTimers()
{
  auto now = std::chrono::system_clock::now();

  auto& timers = pImpl->timers;
  while (!timers.empty() && now >= timers.front().finish) {
    auto front = std::move(timers.front());
    timers.pop_front();
    front.promise.Resolve(Void());
    pImpl->DestroyID(front);
  }
}

bool Timer::RemoveTimer(const uint32_t timerId)
{
  auto& timers = pImpl->timers;
  auto it = std::find_if(
    timers.begin(), timers.end(),
    [timerId](const TimerEntry& entry) { return entry.id == timerId; });
  if (it != timers.end()) {
    timers.erase(it);
    return true;
  }
  return false;
}

Promise<Void> Timer::Set(const std::chrono::system_clock::time_point& endTime,
                         uint32_t* outTimerId)
{
  Promise<Void> promise;
  bool sortRequired =
    !pImpl->timers.empty() && endTime > pImpl->timers.front().finish;

  uint32_t timerId;
  bool created = pImpl->idGenerator->CreateID(timerId);
  if (!created) {
    spdlog::critical("MakeID was not able to Create Id for a timer");
    std::terminate();
  }
  pImpl->timers.push_front({ timerId, promise, endTime });
  if (outTimerId) {
    *outTimerId = timerId;
  }

  if (sortRequired) {
    std::sort(pImpl->timers.begin(), pImpl->timers.end(),
              [](const TimerEntry& lhs, const TimerEntry& rhs) {
                return lhs.finish < rhs.finish;
              });
  }
  return promise;
}

}
