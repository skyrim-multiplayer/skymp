#pragma once

#include <cstddef>

namespace espm {

class IBuffer
{
public:
  virtual ~IBuffer() = default;

  virtual const char* GetData() const = 0;
  virtual size_t GetLength() const = 0;
};

} // namespace espm
