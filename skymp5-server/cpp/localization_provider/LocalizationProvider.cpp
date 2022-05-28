#include "LocalizationProvider.h"
#include <fstream>

DirectoryEntry::DirectoryEntry(uint32_t stringId, uint32_t stringOffset)
  : stringId(stringId)
  , offset(stringOffset)
{
}

std::vector<DirectoryEntry*> LocalizationProvider::ParseDirectoryEntries(
  std::vector<char> buffer)
{
  std::vector<DirectoryEntry*> directoryEntries;
  uint32_t numberOfEntries = *reinterpret_cast<uint32_t*>(&buffer[0]);

  directoryEntries.resize(numberOfEntries);

  uint32_t offset = 8;

  for (int i = 0; i < numberOfEntries; i++) {
    uint32_t stringId = *(uint32_t*)&(buffer[offset]);
    offset += 4;

    uint32_t stringOffset = *(uint32_t*)&(buffer[offset]);
    offset += 4;

    DirectoryEntry* entry = new DirectoryEntry(stringId, stringOffset);
    directoryEntries[i] = entry;
  }

  return directoryEntries;
}

std::map<uint32_t, std::string> LocalizationProvider::ParseStrings(
  std::vector<char> buffer, std::vector<DirectoryEntry*> entries)
{
  std::map<uint32_t, std::string> localization;
  uint32_t numberOfEntries = entries.size();

  for (int i = 0; i < numberOfEntries; i++) {
    uint32_t start = 8 + numberOfEntries * 8 + entries[i]->offset;

    for (int charIndex = 0; charIndex < buffer.size() - start; charIndex++) {
      if (buffer[start + charIndex] == 0) {
        break;
      }

      entries[i]->str += buffer[start + charIndex];
    }

    localization[entries[i]->stringId] = entries[i]->str;
  }

  return localization;
}

std::map<uint32_t, std::string> LocalizationProvider::ParseILDLStrings(
  std::vector<char> buffer, std::vector<DirectoryEntry*> entries)
{
  std::map<uint32_t, std::string> localization;
  uint32_t numberOfEntries = entries.size();

  for (int i = 0; i < numberOfEntries; i++) {
    uint32_t start = 8 + numberOfEntries * 8 + entries[i]->offset;

    entries[i]->length = *(uint32_t*)&(buffer[start]);

    for (int charIndex = 0; charIndex < entries[i]->length - start;
         charIndex++) {
      if (buffer[start + 4 + charIndex] == 0) {
        break;
      }

      entries[i]->str += buffer[start + 4 + charIndex];
    }

    localization[entries[i]->stringId] = entries[i]->str;
  }

  return localization;
}

std::map<uint32_t, std::string> LocalizationProvider::Parse(
  const std::filesystem::directory_entry& file)
{
  std::vector<char> buffer;
  std::ifstream fileStream(file.path(), std::ios::binary);

  buffer.resize(file.file_size());

  if (!fileStream.read(buffer.data(), file.file_size())) {
    throw std::runtime_error("[Localization Provider] can't read " +
                             file.path().string());
  }

  std::vector<DirectoryEntry*> directoryEntries =
    this->ParseDirectoryEntries(buffer);

  if (file.path().extension() == ".strings") {
    return this->ParseStrings(buffer, directoryEntries);
  } else if (file.path().extension() == ".dlstrings" ||
             file.path().extension() == ".ilstrings") {
    return this->ParseILDLStrings(buffer, directoryEntries);
  }

  throw std::runtime_error("Unexcepted file in ./data/strings/ path: " +
                           file.path().filename().string());
}

LocalizationProvider::LocalizationProvider(const std::string& language)
{
  std::string stringsPath = "./data/strings/";

  for (const auto& entry : std::filesystem::directory_iterator(stringsPath)) {
    std::string filename = entry.path().filename().string();

    if (!entry.is_directory() &&
        filename.find(language) != std::string::npos) {
      size_t lastIndex = filename.find_last_of("_");
      std::string name = filename.substr(0, lastIndex);

      auto parsedStrings = this->Parse(entry);

      this->localization[name].insert(parsedStrings.begin(),
                                      parsedStrings.end());
    }
  }
}

std::string LocalizationProvider::Get(const std::string& file, uint32_t stringId)
{
  return this->localization[file][stringId];
}
