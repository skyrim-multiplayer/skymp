#include "Settings.h"

constexpr auto kPlatformSettingsFilePath =
  "Data/SKSE/Plugins/SkyrimPlatform.ini";

std::unique_ptr<Settings::File> Settings::GetPlatformSettings()
{
  auto file = std::make_unique<Settings::File>(kPlatformSettingsFilePath);

  if (!file->IsLoaded()) {
    // if not loaded put some default data and save
    file->SetInteger("Debug", "LogLevel", 2,
                     "; 0 - trace, 1 - debug, 2 - info, 3 - warn, 4 - error, "
                     "5 - critical, 6 - none");
    file->Save();
  }

  return file;
}
