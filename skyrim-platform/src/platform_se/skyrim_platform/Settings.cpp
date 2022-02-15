#include "Settings.h"

constexpr auto platformSettingsFilePath =
  R"(.\Data\SKSE\Plugins\SkyrimPlatform.ini)";

std::unique_ptr<Settings::File> Settings::GetPlatformSettings()
{
  auto file = std::make_unique<Settings::File>(platformSettingsFilePath);

  // load some default data
  if (!file->IsLoaded()) {
    file->data["Debug"].set({ { "LogLevel", "2" } });
    file->Comment("LogLevel", "test comment");
    file->Generate();
  }

  return file;
}
