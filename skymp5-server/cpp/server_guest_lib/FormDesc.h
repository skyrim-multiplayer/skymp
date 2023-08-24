#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

class FormDesc
{
public:
  FormDesc() = default;
  FormDesc(uint32_t shortFormId_, std::string file_)
    : shortFormId(shortFormId_)
    , file(file_)
  {
  }

  std::string ToString(char delimiter = ':') const;
  static FormDesc FromString(std::string str, char delimiter = ':');

  uint32_t ToFormId(const std::vector<std::string>& files) const;
  static FormDesc FromFormId(uint32_t formId,
                             const std::vector<std::string>& files);

  friend bool operator==(const FormDesc& left, const FormDesc& right)
  {
    return std::make_tuple(left.shortFormId, left.file) ==
      std::make_tuple(right.shortFormId, right.file);
  }

  friend bool operator!=(const FormDesc& left, const FormDesc& right)
  {
    return !(left == right);
  }

  friend bool operator<(const FormDesc& left, const FormDesc& right)
  {
    return std::make_tuple(left.shortFormId, left.file) <
      std::make_tuple(right.shortFormId, right.file);
  }

  static FormDesc Tamriel();

  uint32_t shortFormId = 0;
  std::string file;
};
