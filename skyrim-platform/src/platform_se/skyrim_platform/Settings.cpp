#include "Settings.h"

constexpr auto platformSettingsFilePath =
  R"(.\Data\SKSE\Plugins\SkyrimPlatform.ini)";

Settings::File* Settings::GetPlatformSettings()
{
  Settings::File file(platformSettingsFilePath);

  // load some default data
  if (!file.IsLoaded()) {
    file.data["Debug"].set({ { "LogLevel", "2" } });
    file.Generate();
  }

  return &file;
}
