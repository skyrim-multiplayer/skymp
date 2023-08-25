#pragma once
#include <cstdint>

namespace espm {

class RefrKey
{
public:
  RefrKey(uint32_t cellOrWorld, int16_t x, int16_t y);

  operator uint64_t() const noexcept;

private:
  static uint64_t InitValue(uint32_t cellOrWorld, int16_t x_, int16_t y_);

private:
  const uint64_t value;
};

}
