#pragma once
#include <atomic>

class Override
{
public:
  Override();
  Override(const Override&) = delete;
  Override(Override&&) = delete;
  Override& operator=(Override&) = delete;
  Override& operator=(Override&&) = delete;
  ~Override();

  static bool IsOverriden();

private:
  static std::atomic<bool> g_overriden;
};
