#pragma once
#include "IConsolePrinter.h"
#include "JsEngine.h"
#include <spdlog/spdlog.h>

class WindowsConsolePrinter : public IConsolePrinter
{
public:
  WindowsConsolePrinter();
  ~WindowsConsolePrinter() override;

  void Print(const JsFunctionArguments& args) override;
};
