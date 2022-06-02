#include <filesystem>
#include <map>
#include <string>
#include <vector>

class DirectoryEntry
{
public:
  uint32_t stringId;
  uint32_t offset;
  uint32_t length;
  std::string str;

  DirectoryEntry() {}
  DirectoryEntry(uint32_t stringId, uint32_t stringOffset);
};

class LocalizationProvider
{
  std::map<std::string, std::map<uint32_t, std::string>>
    localization; // localization[filename][stringId]

  std::vector<DirectoryEntry> ParseDirectoryEntries(
    const std::vector<char>& buffer);

  void ParseStrings(std::string name, const std::vector<char>& buffer,
                    std::vector<DirectoryEntry>& entries);

  void ParseILDLStrings(std::string name, const std::vector<char>& buffer,
                        std::vector<DirectoryEntry>& entries);

  void Parse(const std::filesystem::directory_entry& file);

public:
  LocalizationProvider(const std::string& language);
  std::string Get(const std::string& file, uint32_t stringId);
};
