#include "TimeUtils.h"
#include <chrono>
#include <iomanip>

std::string TimeUtils::ToString(const TimeUtils::SystemTimePoint& timePoint)
{
  std::stringstream ss;
  std::time_t tp_c = SystemClock::to_time_t(timePoint);
  std::tm now_tm = *std::localtime(&tp_c);
  ss << std::put_time(&now_tm, "%c");
  return ss.str();
}

TimeUtils::SystemTimePoint TimeUtils::SystemTimeFrom(
  const std::string& timestamp)
{
  std::tm tm;
  std::istringstream ss{ timestamp };
  ss >> std::get_time(&tm, "%c");
  return SystemClock::from_time_t(std::mktime(&tm));
}

std::chrono::milliseconds TimeUtils::ToMs(double seconds)
{
  constexpr uint32_t multiplier = 1e3;
  return std::chrono::round<std::chrono::milliseconds>(
    std::chrono::duration<double>{ seconds * multiplier });
}
