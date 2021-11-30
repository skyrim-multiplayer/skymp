class ConsoleHandler
{
public:
  static void PrintConsole(const char* msg...)
  {
    auto console = RE::ConsoleLog::GetSingleton();
    if (!console)
      return;

    va_list args;
    va_start(args, msg);
    console->VPrint(msg, args);
    va_end(args);
  }
};
