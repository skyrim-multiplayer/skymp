#pragma once
#include <cstdint>
#include <string>

#pragma pack(push, 1)

namespace espm {

class Type
{
public:
  // Type object doesn't own this pointer
  explicit Type(const char* type_) noexcept;

  bool operator==(const char* rhs) const noexcept;
  bool operator!=(const char* rhs) const noexcept;
  std::string ToString() const noexcept;
  uint32_t ToUint32() const noexcept;

private:
  const char* type;
};

}

#pragma pack(pop)
