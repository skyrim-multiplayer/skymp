#include "WindowsConsolePrinter.h"

WindowsConsolePrinter::WindowsConsolePrinter()
{
  if (AllocConsole()) {
    freopen("CONOUT$", "w", stdout);

    SetConsoleTitleA("SkyrimPlatform Console");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_GREEN | FOREGROUND_BLUE |
                              FOREGROUND_RED);
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
    if (args[i].GetType() == JsType::Object &&
        !args[i].GetExternalData()) {

      JsValue json = JsValue::GlobalObject().GetProperty("JSON");
      str = json.GetProperty("stringify").Call({ json, args[i] });
    }
    s += str.ToString() + ' ';
  }

  spdlog::info(s);
}
