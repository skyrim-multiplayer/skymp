#include "FormDesc.h"
#include <sstream>

std::string FormDesc::ToString(char delimiter) const
{
  std::stringstream ss;
  ss << std::hex << shortFormId;
  if (!file.empty())
    ss << delimiter << file;
  return ss.str();
}

FormDesc FormDesc::FromString(std::string str, char delimiter)
{
  for (auto& ch : str)
    if (ch == delimiter)
      ch = ' ';

  std::istringstream ss(str);
  FormDesc res;
  ss >> std::hex >> res.shortFormId >> res.file;
  return res;
}

uint32_t FormDesc::ToFormId(const std::vector<std::string>& files) const
{
  uint32_t realFormId;
  if (file.empty()) {
    realFormId = 0xff000000 + shortFormId;
  } else {

    int fileIdx = -1;
    for (int i = 0; i < files.size(); ++i) {
      if (files[i] == file) {
        fileIdx = i;
        break;
      }
    }
    if (fileIdx == -1)
      throw std::runtime_error(file + " not found in loaded files");

    realFormId = fileIdx * 0x01000000 + shortFormId;
  }
  return realFormId;
}

FormDesc FormDesc::FromFormId(uint32_t formId,
                              const std::vector<std::string>& files)
{
  FormDesc res;
  if (formId < 0xff000000) {
    int fileIdx = formId / 0x01000000;
    if (fileIdx >= static_cast<int>(files.size()))
      throw std::runtime_error("FromFormId failed due to invalid file index " +
                               std::to_string(fileIdx));
    res.file = files[fileIdx];
    res.shortFormId = formId % 0x01000000;
  } else {
    res.shortFormId = formId - 0xff000000;
  }
  return res;
}