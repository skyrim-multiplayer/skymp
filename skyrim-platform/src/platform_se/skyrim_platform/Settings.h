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
  bool Generate() { return loaded ? false : file.generate(data); }

  mINI::INIStructure data;

private:
  bool loaded;
  mINI::INIFile file;
};

std::unique_ptr<Settings::File> GetPlatformSettings();
}
