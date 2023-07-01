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

void WindowsConsolePrinter::Print(const JsFunctionArguments& args)
{
  std::string s;

  for (size_t i = 1; i < args.GetSize(); ++i) {
    JsValue str = args[i];
    if (args[i].GetType() == JsValue::Type::Object &&
        !args[i].GetExternalData()) {

      JsValue json = JsValue::GlobalObject().GetProperty("JSON");
      str = json.GetProperty("stringify").Call({ json, args[i] });
    }
    s += str.ToString() + ' ';
  }

  std::cout << s << std::endl;
}
