#include "FormDesc.h"
#include <cstdio>

std::string FormDesc::ToString(char delimiter) const
{
  auto fullFmt = "%0x%c%s";
  auto idFmt = "%0x";
  size_t size = !file.empty()
    ? std::snprintf(nullptr, 0, fullFmt, shortFormId, delimiter, file.c_str())
    : std::snprintf(nullptr, 0, idFmt, shortFormId);

  std::string buffer;
  buffer.resize(size);

  if (!file.empty()) {
    std::sprintf(buffer.data(), fullFmt, shortFormId, delimiter, file.c_str());
  } else {
    std::sprintf(buffer.data(), idFmt, shortFormId);
  }
  return buffer;
}

FormDesc FormDesc::FromString(std::string str, char delimiter)
{
  FormDesc res;
  std::string id, file;

  if (str.find(delimiter) == std::string::npos) {
    std::sscanf(str.data(), "%x", &res.shortFormId);
    return res;
  }

  for (auto it = str.begin(); it != str.end(); ++it) {
    if (*it == delimiter) {
      id = { str.begin(), it };
      res.file = { it + 1, str.end() };
      break;
    }
  }

  std::sscanf(id.data(), "%x", &res.shortFormId);
  return res;
}

uint32_t FormDesc::ToFormId(const std::vector<std::string>& files) const
{
  // Workaround legacy tests throwing exceptions (drop support for PartOne
  // instances without espm to remove this)
  static const std::string kSkyrimEsm = "Skyrim.esm";
  if (shortFormId == 0x3c && file == kSkyrimEsm) {
    return 0x3c;
  }

  uint32_t realFormId;
  if (file.empty()) {
    realFormId = 0xff000000 + shortFormId;
  } else {
    int fileIdx = -1;
    int numFiles = static_cast<int>(files.size());
    for (int i = 0; i < numFiles; ++i) {
      if (files[i] == file) {
        fileIdx = i;
        break;
      }
    }
    if (fileIdx == -1) {
      throw std::runtime_error(file + " not found in loaded files");
    }

    realFormId = fileIdx * 0x01000000 + shortFormId;
  }
  return realFormId;
}

FormDesc FormDesc::FromFormId(uint32_t formId,
                              const std::vector<std::string>& files)
{
  // Workaround legacy tests throwing exceptions (drop support for PartOne
  // instances without espm to remove this)
  if (formId == 0x3c) {
    return FormDesc::Tamriel();
  }

  FormDesc res;
  if (formId < 0xff000000) {
    int fileIdx = formId / 0x01000000;
    if (fileIdx >= static_cast<int>(files.size())) {
      throw std::runtime_error("FromFormId failed due to invalid file index " +
                               std::to_string(fileIdx));
    }
    res.file = files[fileIdx];
    res.shortFormId = formId % 0x01000000;
  } else {
    res.shortFormId = formId - 0xff000000;
  }
  return res;
}

static const FormDesc kTamriel = FormDesc::FromString("3c:Skyrim.esm");

FormDesc FormDesc::Tamriel()
{
  return kTamriel;
}
