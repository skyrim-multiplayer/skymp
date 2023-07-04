#pragma once
#include <chrono>
#include <sstream>

#include <iostream>

namespace Viet {

class TimeUtils
{
public:
  static std::string ToString(
    const std::chrono::system_clock::time_point& timePoint);
  static std::chrono::system_clock::time_point SystemTimeFrom(
    const std::string& timestamp);

  template <typename DurationType>
  static DurationType To(double seconds)
  {
    auto time = std::chrono::round<DurationType>(
      std::chrono::duration<double>{ seconds });
    return time;
  }
};

}
