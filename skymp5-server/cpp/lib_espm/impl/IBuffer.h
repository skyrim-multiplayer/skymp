#pragma once

#include <cstddef>

namespace espm::impl {

class IBuffer
{
public:
  virtual ~IBuffer() = default;

  virtual char* GetData() = 0;
  virtual size_t GetLength() = 0;
};

} // namespace espm::impl
