#include "WindowsConsolePrinter.h"

#include <Windows.h>

WindowsConsolePrinter::WindowsConsolePrinter(int offsetLeft, int offsetTop,
                                             int width, int height,
                                             bool isAlwaysOnTop)
{
  if (AllocConsole()) {
    freopen("CONOUT$", "w", stdout);

    SetConsoleTitleA("SkyrimPlatform Console");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_GREEN | FOREGROUND_BLUE |
                              FOREGROUND_RED);

    HWND window_header = GetConsoleWindow();

    SetWindowPos(window_header, isAlwaysOnTop ? HWND_TOPMOST : HWND_TOP,
                 offsetLeft, offsetTop, width, height, 0x4000);
  }
}

WindowsConsolePrinter::~WindowsConsolePrinter()
{
  FreeConsole();
}

void WindowsConsolePrinter::Print(const Napi::CallbackInfo& info)
{
  std::string s;

  for (size_t i = 0; i < info.Length(); ++i) {
    Napi::Value str = info[i];

    if (info[i].IsObject() && !info[i].IsExternal()) {
      Napi::Object global = env.Global();
      Napi::Object json = global.Get("JSON").As<Napi::Object>();
      Napi::Function stringify = json.Get("stringify").As<Napi::Function>();
      str = stringify.Call(json, { info[i] });
    }

    s += str.ToString().Utf8Value() + " ";
  }

  std::cout << s << std::endl;
}

void WindowsConsolePrinter::PrintRaw(const char* str)
{
  std::cout << str << std::endl;
}
