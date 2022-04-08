#pragma once

namespace Settings {

enum class Section
{
  General,
  Debug,
};

/**
 * TODO: add array support for config files
 */
class File
{
public:
  File(const char* _path)
    : path(_path)
  {
    ini.SetUnicode();
    loadStatus = ini.LoadFile(path);
  }

  ~File()
  {
    if (changed) {
      Save();
    }
  }

  const char* GetFilePath() const { return path; }

  bool IsLoaded() const { return loadStatus == SI_OK; }

  bool Reload()
  {
    loadStatus = ini.LoadFile(path);
    return IsLoaded();
  }

  bool Save()
  {
    const auto status = ini.SaveFile(path);
    const auto success = status >= 0;
    if (success) {
      loadStatus = status;
      changed = false;
    }
    return success;
  }

  long GetInteger(const char* section, const char* key, long defaultValue)
  {
    return ini.GetLongValue(section, key, defaultValue);
  }

  template <IntegralOrEnum T>
  T GetInteger(const char* section, const char* key, T defaultValue)
  {
    return (T)GetInteger(section, key, (long)defaultValue);
  }

  bool SetInteger(const char* section, const char* key, long value,
                  const char* comment = nullptr)
  {
    auto status = ini.SetLongValue(section, key, value, comment);
    if (status == SI_FAIL) {
      logger::info("Failed to set config value. file - {}, section - {}, key "
                   "- {}, value - {}.",
                   GetFilePath(), section, key, value);
    } else if (status == SI_INSERTED) {
      logger::debug("Inserted config key. file - {}, section - {}, key "
                    "- {}, value - {}.",
                    GetFilePath(), section, key, value);
    } else if (status == SI_UPDATED) {
      logger::debug("Updated config key. file - {}, section - {}, key "
                    "- {}, value - {}.",
                    GetFilePath(), section, key, value);
    }

    auto success = status >= 0;
    if (success) {
      changed = true;
    }
    return success;
  }

  template <IntegralOrEnum T>
  bool SetInteger(const char* section, const char* key, T value,
                  const char* comment = nullptr)
  {
    return SetInteger(section, key, (long)value, comment);
  }

  double GetFloat(const char* section, const char* key, double defaultValue)
  {
    return ini.GetDoubleValue(section, key, defaultValue);
  }

  template <FloatingPoint T>
  T GetFloat(const char* section, const char* key, T defaultValue)
  {
    return (T)GetFloat(section, key, (double)defaultValue);
  }

  bool SetFloat(const char* section, const char* key, double value,
                const char* comment = nullptr)
  {
    auto status = ini.SetDoubleValue(section, key, value, comment);
    if (status == SI_FAIL) {
      logger::info("Failed to set config value. file - {}, section - {}, key "
                   "- {}, value - {}.",
                   GetFilePath(), section, key, value);
    } else if (status == SI_INSERTED) {
      logger::debug("Inserted config key. file - {}, section - {}, key "
                    "- {}, value - {}.",
                    GetFilePath(), section, key, value);
    } else if (status == SI_UPDATED) {
      logger::debug("Updated config key. file - {}, section - {}, key "
                    "- {}, value - {}.",
                    GetFilePath(), section, key, value);
    }

    auto success = status >= 0;
    if (success) {
      changed = true;
    }
    return success;
  }

  template <FloatingPoint T>
  bool SetFloat(const char* section, const char* key, T value,
                const char* comment = nullptr)
  {
    return SetFloat(section, key, (double)value, comment);
  }

  bool GetBool(const char* section, const char* key, bool defaultValue)
  {
    return ini.GetBoolValue(section, key, defaultValue);
  }

  bool SetBool(const char* section, const char* key, bool value,
               const char* comment = nullptr)
  {
    auto status = ini.SetBoolValue(section, key, value, comment);
    if (status == SI_FAIL) {
      logger::info("Failed to set config value. file - {}, section - {}, key "
                   "- {}, value - {}.",
                   GetFilePath(), section, key, value);
    } else if (status == SI_INSERTED) {
      logger::debug("Inserted config key. file - {}, section - {}, key "
                    "- {}, value - {}.",
                    GetFilePath(), section, key, value);
    } else if (status == SI_UPDATED) {
      logger::debug("Updated config key. file - {}, section - {}, key "
                    "- {}, value - {}.",
                    GetFilePath(), section, key, value);
    }

    auto success = status >= 0;
    if (success) {
      changed = true;
    }
    return success;
  }

  const char* GetString(const char* section, const char* key,
                        const char* defaultValue)
  {
    return ini.GetValue(section, key, defaultValue);
  }

  bool SetString(const char* section, const char* key, const char* value,
                 const char* comment = nullptr)
  {
    auto status = ini.SetValue(section, key, value, comment);
    if (status == SI_FAIL) {
      logger::info("Failed to set config value. file - {}, section - {}, key "
                   "- {}, value - {}.",
                   GetFilePath(), section, key, value);
    } else if (status == SI_INSERTED) {
      logger::debug("Inserted config key. file - {}, section - {}, key "
                    "- {}, value - {}.",
                    GetFilePath(), section, key, value);
    } else if (status == SI_UPDATED) {
      logger::debug("Updated config key. file - {}, section - {}, key "
                    "- {}, value - {}.",
                    GetFilePath(), section, key, value);
    }

    auto success = status >= 0;
    if (success) {
      changed = true;
    }
    return success;
  }

  bool DeleteKey(const char* section, const char* key,
                 bool deleteEmptySection = true)
  {
    auto success = ini.Delete(section, key, deleteEmptySection);

    if (success) {
      changed = true;
    }
    return success;
  }

  std::string ToString()
  {
    std::string fileContent;
    ini.Save(fileContent);
    return fileContent;
  }

private:
  File() = delete;
  File(const File&) = delete;
  File(File&&) = delete;

  CSimpleIniA ini;
  const char* path = "";
  int loadStatus = 0;
  bool changed = false;
};

std::unique_ptr<Settings::File> GetPlatformSettings();
}
