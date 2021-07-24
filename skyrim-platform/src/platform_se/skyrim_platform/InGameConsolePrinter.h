#pragma once
#include "IConsolePrinter.h"

class InGameConsolePrinter : public IConsolePrinter
{
public:
  void Print(const JsFunctionArguments& args) override;
};
