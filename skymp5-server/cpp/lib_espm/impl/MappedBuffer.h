#pragma once

#include <filesystem>

#include "../Loader.h"
#include "IBuffer.h"

namespace espm::impl {

class MappedBuffer : public impl::IBuffer
{
public:
  MappedBuffer(const fs::path& path);

  ~MappedBuffer();

  char* GetData() override { return data_; }

  size_t GetLength() override { return size_; }

private:
  int fd_;
  char* data_;
  size_t size_;
};

} // namespace espm::impl
