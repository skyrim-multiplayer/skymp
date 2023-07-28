#include "TimeUtils.h"
#include <chrono>
#include <iomanip>

namespace Viet {

std::string TimeUtils::ToString(
  const std::chrono::system_clock::time_point& timePoint)
{
  std::stringstream ss;
  std::time_t tp_c = std::chrono::system_clock::to_time_t(timePoint);
  std::tm now_tm = *std::localtime(&tp_c);
  ss << std::put_time(&now_tm, "%c");
  return ss.str();
}

std::chrono::system_clock::time_point TimeUtils::SystemTimeFrom(
  const std::string& timestamp)
{
  std::tm tm;
  std::istringstream ss{ timestamp };
  ss >> std::get_time(&tm, "%c");
  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

}
