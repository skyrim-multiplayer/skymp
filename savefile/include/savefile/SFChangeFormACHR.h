#pragma once
#include <cstdint>
#include <utility>
#include <vector>

namespace SaveFile_ {
struct ChangeFormACHR_
{
  std::vector<uint8_t> data;
  std::pair<uint32_t, std::vector<uint8_t>> ToBinary() const noexcept;
};
}
