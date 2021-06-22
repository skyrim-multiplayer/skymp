#include <Windows.h>
#include <filesystem>
#include <stdio.h>
#include <string>

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine,
            INT nCmdShow)
{
  auto root = std::filesystem::current_path().parent_path();
  auto p = (root / "tools/dev_service").wstring();
  SetCurrentDirectoryW(p.data());

  std::string cmd;
#ifdef NDEBUG
  cmd = "npm run start";
#else
  cmd = "npm run start-debug";
#endif
  cmd += " || pause";

  system(cmd.data());
  return 0;
}