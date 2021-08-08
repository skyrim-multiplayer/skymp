#include "MappedBuffer.h"

#include <fmt/format.h>

#ifdef WIN32
#else
// Linux
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace espm::impl {

MappedBuffer::MappedBuffer(const fs::path& path)
{
  size_ = fs::file_size(path);
  fd_ = open(path.c_str(), O_RDONLY);
  if (fd_ == -1) {
    throw std::system_error(errno, std::generic_category(),
                            fmt::format("Can't read {}", path.string()));
  }
  const auto mmapResult = mmap(NULL, size_, PROT_READ, MAP_SHARED, fd_, 0);
  if (mmapResult == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category(),
                            fmt::format("Can't map {}", path.string()));
  }
  data_ = static_cast<char*>(mmapResult);
}

MappedBuffer::~MappedBuffer()
{
  int result = munmap(data_, size_);
  if (result) {
    abort();
  }
}

} // namespace espm::impl
