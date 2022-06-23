#pragma once
#include "IConsolePrinter.h"

class WindowsConsolePrinter : public IConsolePrinter
{
public:
  WindowsConsolePrinter();
  ~WindowsConsolePrinter() override;

  void Print(const JsFunctionArguments& args) override;
};
