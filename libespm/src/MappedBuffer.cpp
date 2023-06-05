#include "libespm/MappedBuffer.h"

#include <iostream>

#ifndef WIN32
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <unistd.h>
#endif

namespace espm {

MappedBuffer::MappedBuffer(const fs::path& path)
{
  size_ = fs::file_size(path);
#ifdef WIN32
  fileHandle_ = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, 0);
  if (!fileHandle_) {
    throw std::system_error(GetLastError(), std::system_category(),
                            "[espm] CreateFileW failed for " + path.string());
  }

  mapHandle_ = CreateFileMapping(fileHandle_, NULL, PAGE_READONLY, 0, 0, NULL);
  if (!mapHandle_) {
    throw std::system_error(GetLastError(), std::system_category(),
                            "[espm] CreateFileMapping failed for " +
                              path.string());
  }

  viewPtr_ = MapViewOfFile(mapHandle_, FILE_MAP_READ, 0, 0, 0);
  if (!mapHandle_) {
    throw std::system_error(GetLastError(), std::system_category(),
                            "[espm] CreateFileMapping failed for " +
                              path.string());
  }
  data_ = static_cast<char*>(viewPtr_);
#else
  fd_ = open(path.c_str(), O_RDONLY);
  if (fd_ == -1) {
    throw std::system_error(errno, std::system_category(),
                            "[espm] open failed for " + path.string());
  }
  const auto mmapResult = mmap(NULL, size_, PROT_READ, MAP_SHARED, fd_, 0);
  if (mmapResult == MAP_FAILED) {
    throw std::system_error(errno, std::system_category(),
                            "[espm] mmap failed for " + path.string());
  }
  data_ = static_cast<char*>(mmapResult);
#endif
}

MappedBuffer::~MappedBuffer()
{
#ifdef WIN32
  if (viewPtr_) {
    bool result = UnmapViewOfFile(viewPtr_);
    if (!result) {
      std::cerr << "[espm] can't unmap file, error=" << GetLastError()
                << std::endl;
      std::terminate();
    }
  }
  if (mapHandle_) {
    bool result = CloseHandle(mapHandle_);
    if (!result) {
      std::cerr << "[espm] can't close map handle, error=" << GetLastError()
                << std::endl;
      std::terminate();
    }
  }
  if (fileHandle_) {
    bool result = CloseHandle(fileHandle_);
    if (!result) {
      std::cerr << "[espm] can't close file handle, error=" << GetLastError()
                << std::endl;
      std::terminate();
    }
  }
#else
  if (data_) {
    int result = munmap(data_, size_);
    if (result == -1) {
      std::cerr << "[espm] can't unmap file, errno=" << errno << std::endl;
      std::terminate();
    }
  }
  if (fd_ != -1) {
    int result = close(fd_);
    if (result == -1) {
      std::cerr << "[espm] can't close file, errno=" << errno << std::endl;
      std::terminate();
    }
  }
#endif
}

const char* MappedBuffer::GetData() const
{
  return data_;
}

size_t MappedBuffer::GetLength() const
{
  return size_;
}

} // namespace espm
