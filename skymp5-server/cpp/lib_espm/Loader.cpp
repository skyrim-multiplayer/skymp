#include "Loader.h"

#include "impl/AllocatedBuffer.h"
#include "impl/MappedBuffer.h"

namespace espm {

std::unique_ptr<impl::IBuffer> Loader::MakeBuffer(
  const fs::path& filePath) const
{
  //return std::unique_ptr<impl::IBuffer>{new impl::AllocatedBuffer(filePath)};
  return std::unique_ptr<impl::IBuffer>{ new impl::MappedBuffer(filePath) };
}

} // namespace espm
