#include "LocalizationProvider.h"
#include <fstream>

DirectoryEntry::DirectoryEntry(uint32_t stringId, uint32_t stringOffset)
  : stringId(stringId)
  , offset(stringOffset)
{
}

void LocalizationProvider::ParseDirectoryEntries(
  const std::vector<char>& buffer, std::vector<DirectoryEntry>& entries)
{
  uint32_t numberOfEntries = *reinterpret_cast<const uint32_t*>(&buffer[0]);

  entries.clear();
  entries.resize(numberOfEntries);

  uint32_t offset = 8;

  for (int i = 0; i < numberOfEntries; i++) {
    uint32_t stringId = *reinterpret_cast<const uint32_t*>(&buffer[offset]);
    offset += 4;

    uint32_t stringOffset =
      *reinterpret_cast<const uint32_t*>(&buffer[offset]);
    offset += 4;

    DirectoryEntry entry(stringId, stringOffset);
    entries[i] = entry;
  }
}

void LocalizationProvider::ParseStrings(std::string name,
                                        const std::vector<char>& buffer,
                                        std::vector<DirectoryEntry>& entries)
{
  uint32_t numberOfEntries = entries.size();

  for (int i = 0; i < numberOfEntries; i++) {
    uint32_t start = 8 + numberOfEntries * 8 + entries[i].offset;

    for (int charIndex = 0; charIndex < buffer.size() - start; charIndex++) {
      if (buffer[start + charIndex] == 0) {
        break;
      }

      entries[i].str += buffer[start + charIndex];
    }

    localization[name][entries[i].stringId] = entries[i].str;
  }
}

void LocalizationProvider::ParseILDLStrings(
  std::string name, const std::vector<char>& buffer,
  std::vector<DirectoryEntry>& entries)
{
  uint32_t numberOfEntries = entries.size();

  for (int i = 0; i < numberOfEntries; i++) {
    uint32_t start = 8 + numberOfEntries * 8 + entries[i].offset;

    entries[i].length = static_cast<uint32_t>(buffer[start]);

    for (int charIndex = 0; charIndex < entries[i].length - start;
         charIndex++) {
      if (buffer[start + 4 + charIndex] == 0) {
        break;
      }

      entries[i].str += buffer[start + 4 + charIndex];
    }

    localization[name][entries[i].stringId] = entries[i].str;
  }
}

void LocalizationProvider::Parse(const std::filesystem::directory_entry& file)
{
  std::string filename = file.path().filename().string();
  size_t lastIndex = filename.find_last_of("_");
  std::string name = filename.substr(0, lastIndex);

  std::vector<char> buffer;
  std::ifstream fileStream(file.path(), std::ios::binary);

  buffer.resize(file.file_size());

  if (!fileStream.read(buffer.data(), file.file_size())) {
    return;
  }

  std::vector<DirectoryEntry> directoryEntries;
  ParseDirectoryEntries(buffer, directoryEntries);

  if (file.path().extension() == ".strings") {
    ParseStrings(name, buffer, directoryEntries);
  } else if (file.path().extension() == ".dlstrings" ||
             file.path().extension() == ".ilstrings") {
    ParseILDLStrings(name, buffer, directoryEntries);
  }
}

LocalizationProvider::LocalizationProvider(const std::string& dataDir,
                                           const std::string& language)
{
  std::filesystem::path stringsPath =
    std::filesystem::path(dataDir) / "strings";

  if (!std::filesystem::exists(stringsPath)) {
    return;
  }

  for (const auto& entry : std::filesystem::directory_iterator(stringsPath)) {
    std::string filename = entry.path().filename().string();

    if (!entry.is_directory() &&
        filename.find(language) != std::string::npos) {

      Parse(entry);
    }
  }
}

const std::string& LocalizationProvider::Get(const std::string& file,
                                             uint32_t stringId)
{
  return localization[file][stringId];
}
