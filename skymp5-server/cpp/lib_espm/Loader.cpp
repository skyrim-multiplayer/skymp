#include "Loader.h"

#include "impl/AllocatedBuffer.h"
#include "impl/MappedBuffer.h"

namespace espm {

std::unique_ptr<impl::IBuffer> Loader::MakeBuffer(
  const fs::path& filePath) const
{
  switch (bufferType) {
    case BufferType::AllocatedBuffer:
      return std::make_unique<impl::AllocatedBuffer>(filePath);
    case BufferType::MappedBuffer:
      return std::make_unique<impl::MappedBuffer>(filePath);
    default:
      throw std::runtime_error("[espm] unhandled buffer type");
  }
}

} // namespace espm
