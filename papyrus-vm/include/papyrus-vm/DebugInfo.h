#pragma once
#include <cstdint>
#include <vector>
#include <string>

struct DebugInfo
{
  uint8_t m_flags = 0;
  uint64_t m_sourceModificationTime = 0;

  struct DebugFunction
  {
    std::string objName;
    std::string stateName;
    std::string fnName;
    uint8_t type = 0; // 0-3 valid

    std::vector<uint16_t> lineNumbers; // one per instruction

    size_t GetNumInstructions() { return lineNumbers.size(); }
  };

  std::vector<DebugFunction> m_data;
};
