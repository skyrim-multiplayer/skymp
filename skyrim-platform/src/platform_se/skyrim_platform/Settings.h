#pragma once

namespace Settings {

enum class Section
{
  General,
  Debug,
};

class File
{
public:
  File(const std::string& filepath)
    : file(mINI::INIFile(filepath))
  {
    loaded = file.read(data);
  }

  bool IsLoaded() { return loaded; }
  void Reload() { loaded = file.read(data); }
  bool Save() { return loaded ? file.write(data) : loaded; }
  bool Generate() { return loaded ? false : file.generate(data, &comment); }
  void Comment(std::string key, std::string text)
  {
    comment.emplace(key, text);
  }

  mINI::INIStructure data;

private:
  bool loaded;
  mINI::INIFile file;
  std::unordered_map<std::string, std::string> comment;
};

std::unique_ptr<Settings::File> GetPlatformSettings();
}
