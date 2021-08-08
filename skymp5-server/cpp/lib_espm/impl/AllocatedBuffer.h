#pragma once

#include <filesystem>

#include <fmt/format.h>

#include "../Loader.h"
#include "IBuffer.h"

namespace espm::impl {

class AllocatedBuffer : public IBuffer
{
public:
  AllocatedBuffer(const std::filesystem::path& path)
    : data()
  {
    const auto size = std::filesystem::file_size(path);
    data.resize(size);

    std::ifstream f(path.string(), std::ios::binary);
    if (!f.read(data.data(), size)) {
      throw Loader::LoadError(fmt::format("Can't read {}", path.string()));
    }
  }

  char* GetData() override { return data.data(); }

  size_t GetLength() override { return data.size(); }

private:
  std::vector<char> data;
};

} // namespace espm::impl
