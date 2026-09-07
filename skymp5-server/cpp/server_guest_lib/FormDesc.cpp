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
  buffer.resize(size + 1);

  if (!file.empty()) {
    std::sprintf(buffer.data(), fullFmt, shortFormId, delimiter, file.c_str());
  } else {
    std::sprintf(buffer.data(), idFmt, shortFormId);
  }
  buffer.resize(size); // remove extra null terminator
  return buffer;
}

FormDesc FormDesc::FromString(const std::string& str, char delimiter)
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

uint32_t FormDesc::ToFormId(const EspmFileTable& files) const
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

    // Light (ESL) plugins live in the 0xFE space with a 12-bit local id
    const espm::PluginSlot slot = files.SlotAt(fileIdx);
    if (slot.light && shortFormId > slot.LocalMask()) {
      // Truncating would silently resolve to an unrelated record
      throw std::runtime_error(
        file + " is a light plugin but form id " + std::to_string(shortFormId) +
        " does not fit its 12-bit local id space");
    }
    realFormId = slot.Base() | (shortFormId & slot.LocalMask());
  }
  return realFormId;
}

FormDesc FormDesc::FromFormId(uint32_t formId, const EspmFileTable& files)
{
  // Workaround legacy tests throwing exceptions (drop support for PartOne
  // instances without espm to remove this)
  if (formId == 0x3c) {
    return FormDesc::Tamriel();
  }

  FormDesc res;
  if (formId >= 0xff000000) {
    res.shortFormId = formId - 0xff000000;
    return res;
  }

  // Light plugins share the 0xFE high byte, so resolve them by slot. This must
  // run before the byte-index path below, which would read 254 as a file index.
  const int fileIdx = files.FileIndexOf(formId);
  if (fileIdx < 0) {
    throw std::runtime_error("FromFormId failed due to invalid file index " +
                             std::to_string(formId >> 24));
  }
  res.file = files[fileIdx];
  res.shortFormId = formId & files.SlotAt(fileIdx).LocalMask();
  return res;
}

static const FormDesc kTamriel = FormDesc::FromString("3c:Skyrim.esm");

FormDesc FormDesc::Tamriel()
{
  return kTamriel;
}
