#pragma once
#include "IConsolePrinter.h"

class WindowsConsolePrinter : public IConsolePrinter
{
public:
  WindowsConsolePrinter(int offsetLeft, int offsetTop, int width, int height,
                        bool isAlwaysOnTop);
  ~WindowsConsolePrinter() override;

  void Print(const JsFunctionArguments& args) override;
};
