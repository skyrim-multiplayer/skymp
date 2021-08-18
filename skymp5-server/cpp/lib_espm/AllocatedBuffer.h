#pragma once

#include <filesystem>

#include "IBuffer.h"
#include "Loader.h"

namespace espm {

class AllocatedBuffer : public IBuffer
{
public:
  AllocatedBuffer(const std::filesystem::path& path);

  const char* GetData() const override;

  size_t GetLength() const override;

private:
  std::vector<char> data;
};

} // namespace espm
