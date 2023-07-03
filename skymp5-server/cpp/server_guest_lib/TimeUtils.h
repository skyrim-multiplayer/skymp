#pragma once
#include <chrono>
#include <sstream>

class TimeUtils
{
private:
  using SystemClock = std::chrono::system_clock;
  using SystemTimePoint = std::chrono::system_clock::time_point;

public:
  static std::string ToString(const SystemTimePoint& timePoint);
  static SystemTimePoint SystemTimeFrom(const std::string& timestamp);
  static std::chrono::milliseconds ToMs(double seconds);
};
