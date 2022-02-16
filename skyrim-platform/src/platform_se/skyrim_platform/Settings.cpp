#include "Settings.h"

constexpr auto platformSettingsFilePath =
  R"(.\Data\SKSE\Plugins\SkyrimPlatform.ini)";

std::unique_ptr<Settings::File> Settings::GetPlatformSettings()
{
  auto file = std::make_unique<Settings::File>(platformSettingsFilePath);

  // load some default data
  if (!file->IsLoaded()) {
    file->SetInteger("Debug", "LogLevel", 2,
                     "; 0 - trace, 1 - debug, 2 - info, 3 - warn, 4 - error, "
                     "5- critical, 6 - none");
    file->Save();
  }

  return file;
}
