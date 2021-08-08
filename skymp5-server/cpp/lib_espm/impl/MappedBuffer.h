#pragma once

#include <filesystem>

#ifdef WIN32
#  include <Windows.h>
#endif

#include "../Loader.h"
#include "IBuffer.h"

namespace espm::impl {

class MappedBuffer : public impl::IBuffer
{
public:
  MappedBuffer(const fs::path& path);

  ~MappedBuffer();

  char* GetData() override;

  size_t GetLength() override;

private:
#ifdef WIN32
  HANDLE fileHandle_ = nullptr;
  HANDLE mapHandle_ = nullptr;
  LPVOID viewPtr_ = nullptr;
#else
  int fd_;
#endif
  char* data_;
  size_t size_;
};

} // namespace espm::impl
