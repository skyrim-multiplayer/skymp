#pragma once
#include <cstdint>
#include <string>

class UserFlagView
{
public:
  UserFlagView(const uint8_t* data_, size_t dataSize_,
               const StringTable* stringTable_) noexcept;

  const std::string& GetName() const noexcept;
  uint8_t GetIdx() const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

// struct UserFlag
// {
//   std::string name;
//   uint8_t idx = 0;
// };
