#pragma once

#include <filesystem>

#include <fmt/format.h>

#include "../Loader.h"
#include "IBuffer.h"

namespace espm::impl {

class AllocatedBuffer : public IBuffer
{
public:
  AllocatedBuffer(const std::filesystem::path& path);

  char* GetData() override;

  size_t GetLength() override;

private:
  std::vector<char> data;
};

} // namespace espm::impl
