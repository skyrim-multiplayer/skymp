#include "AllocatedBuffer.h"

namespace espm::impl {

AllocatedBuffer::AllocatedBuffer(const std::filesystem::path& path)
  : data()
{
  const auto size = std::filesystem::file_size(path);
  data.resize(size);

  std::ifstream f(path.string(), std::ios::binary);
  if (!f.read(data.data(), size)) {
    throw Loader::LoadError(fmt::format("Can't read {}", path.string()));
  }
}

char* AllocatedBuffer::GetData()
{
  return data.data();
}

size_t AllocatedBuffer::GetLength()
{
  return data.size();
}

} // namespace espm::impl
