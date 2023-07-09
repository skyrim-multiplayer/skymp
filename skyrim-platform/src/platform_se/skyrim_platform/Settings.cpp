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

    file->SetBool(
      "Debug", "Cmd", false,
      "; If enabled, it will run the cmd with the game console data");

    file->SetInteger("Debug", "CmdOffsetLeft", 0, "; Set left offset for cmd");

    file->SetInteger("Debug", "CmdOffsetTop", 720, "; Set top offset for cmd");

    file->SetInteger("Debug", "CmdWidth", 1900, "; Set width for cmd");

    file->SetInteger("Debug", "CmdHeight", 317, "; Set height for cmd");

    file->SetBool("Debug", "CmdIsAlwaysOnTop", false,
                  "; Set Cmd stay on top of others windows");

    file->Save();
  }

  return file;
}
