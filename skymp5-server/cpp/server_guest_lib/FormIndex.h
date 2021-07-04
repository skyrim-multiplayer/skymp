#pragma once
#include <cstdint>

class FormIndex
{
public:
  constexpr static uint32_t g_invalidIdx = (uint32_t)-1;

  const auto& GetIdx() { return idx; }

  uint32_t idx = g_invalidIdx;
};
