#include "savefile/SFChangeFormACHR.h"
#include "savefile/SFTemplates.h"

#include <string>

namespace {
bool KeyCheck(const std::vector<uint8_t>& vec, const uint32_t& startIndex,
              const std::string& key)
{
  std::string res;

  for (size_t i = startIndex; i < key.size() + startIndex; ++i)
    res += char(vec[i]);

  return res == key;
}

template <class T>
void ValueNewWrite(std::vector<uint8_t>& vec, const uint32_t startIndex,
                   T& newValue)
{

  if (sizeof(newValue) < 4 || sizeof(newValue) > 4)
    return;

  uint8_t* value = reinterpret_cast<uint8_t*>(&newValue);

  for (size_t i = 0; i < 4; ++i)
    vec[startIndex + i] = value[i];
}

template <class T>
bool KeyForValueChange(std::vector<uint8_t>& vec, const std::string& key,
                       T& newValue)
{

  for (size_t i = 0; i < vec.size(); ++i) {
    if (vec[i] != key.size())
      continue;

    if (vec[i + 1] == 0x00 ? KeyCheck(vec, i + 2, key)
                           : KeyCheck(vec, i + 1, key)) {
      const uint32_t startIndex =
        vec[i + 1] == 0x00 ? (key.size() + 1 + i) : (key.size() + i);
      ValueNewWrite(vec, startIndex + 2, newValue);
      return true;
    }
  }
  return false;
}
}

std::pair<uint32_t, std::vector<uint8_t>>
SaveFile_::ChangeFormACHR_::ToBinary() const noexcept
{
  std::pair<uint32_t, std::vector<uint8_t>> res;

  res.first = 2550136834;
  res.second = Templates::ACHR;

  const std::string bodyMorph = "bodyMorphWeight";
  const std::string bodyMuscular = "bodyMorphMuscular";

  float newValueMorph = 0x333333f3;
  float newValueMuscular = 0x9a999e30;

  KeyForValueChange(res.second, bodyMorph, newValueMorph);
  KeyForValueChange(res.second, bodyMuscular, newValueMuscular);

  return res;
}
