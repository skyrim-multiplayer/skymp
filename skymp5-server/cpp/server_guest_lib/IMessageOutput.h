#pragma once
#include <cstdint>
#include <cwchar>

class IMessageOutput
{
public:
  virtual ~IMessageOutput() = default;

  virtual void Send(const uint8_t* data, size_t length, bool reliable) = 0;
};
