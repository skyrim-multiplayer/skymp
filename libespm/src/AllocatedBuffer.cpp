#include "libespm/AllocatedBuffer.h"

namespace espm {

AllocatedBuffer::AllocatedBuffer(const std::filesystem::path& path)
  : data()
{
  const auto size = std::filesystem::file_size(path);
  data.resize(size);

  std::ifstream f(path.string(), std::ios::binary);
  if (!f.read(data.data(), size)) {
    throw std::runtime_error("[espm] can't read " + path.string());
  }
}

const char* AllocatedBuffer::GetData() const
{
  return data.data();
}

size_t AllocatedBuffer::GetLength() const
{
  return data.size();
}

} // namespace espm
