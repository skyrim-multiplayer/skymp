#pragma once

#include <span>
#include <vector>

struct AnimVariableMasterGraphIndexes final
{
  explicit AnimVariableMasterGraphIndexes(
    const std::span<uint32_t>& intVariableIndexes_,
    const std::span<uint32_t>& floatVariableIndexes_,
    const std::span<uint32_t>& boolVariableIndexes_)
    : intVariableIndexes(intVariableIndexes_.begin(),
                         intVariableIndexes_.end())
    , floatVariableIndexes(floatVariableIndexes_.begin(),
                           floatVariableIndexes_.end())
    , boolVariableIndexes(boolVariableIndexes_.begin(),
                          boolVariableIndexes_.end())
  {
  }

  [[nodiscard]] static AnimVariableMasterGraphIndexes CreateDefault();

public:
  std::vector<uint32_t> intVariableIndexes{}, floatVariableIndexes{},
    boolVariableIndexes{};
};
