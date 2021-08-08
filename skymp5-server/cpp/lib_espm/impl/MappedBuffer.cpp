#include "MappedBuffer.h"

#include <fmt/format.h>

#ifndef WIN32
// Linux
#  include <fcntl.h>
#  include <sys/mman.h>
#endif

namespace espm::impl {

MappedBuffer::MappedBuffer(const fs::path& path)
{
  size_ = fs::file_size(path);
#ifdef WIN32
  fileHandle_ = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, 0);
  if (!fileHandle_) {
    // generic vs system?
    throw std::system_error(
      std::error_code(GetLastError(), std::system_category()),
      "CreateFileW failed");
  }

  mapHandle_ = CreateFileMapping(fileHandle_, NULL, PAGE_READONLY, 0, 0, NULL);
  if (!mapHandle_) {
    throw std::system_error(
      std::error_code(GetLastError(), std::system_category()),
      "CreateFileMapping failed");
  }

  viewPtr_ = MapViewOfFileEx(mapHandle_, FILE_MAP_READ, 0, 0, 0, NULL);
  if (!mapHandle_) {
    throw std::system_error(
      std::error_code(GetLastError(), std::system_category()),
      "CreateFileMapping failed");
  }
  data_ = static_cast<char*>(viewPtr_);
#else
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
#endif
}

MappedBuffer::~MappedBuffer()
{
#ifdef WIN32
  if (viewPtr_) {
    UnmapViewOfFile(viewPtr_);
  }
  if (mapHandle_) {
    CloseHandle(mapHandle_);
  }
  if (fileHandle_) {
    CloseHandle(fileHandle_);
  }
#else
  if (!data_) {
    return;
  }
  int result = munmap(data_, size_);
  if (result) {
    abort();
  }
#endif
}

char* MappedBuffer::GetData()
{
  return data_;
}

size_t MappedBuffer::GetLength()
{
  return size_;
}

} // namespace espm::impl
