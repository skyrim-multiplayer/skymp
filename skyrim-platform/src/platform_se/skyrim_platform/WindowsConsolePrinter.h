#pragma once
#include "IConsolePrinter.h"

class WindowsConsolePrinter : public IConsolePrinter
{
public:
  WindowsConsolePrinter(const int offsetLeft, const int offsetTop,
                        const int width, const int height,
                        const bool isAlwaysOnTop);
  ~WindowsConsolePrinter() override;

  void Print(const JsFunctionArguments& args) override;
};
