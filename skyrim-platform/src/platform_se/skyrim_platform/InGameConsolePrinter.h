#pragma once
#include "IConsolePrinter.h"

class InGameConsolePrinter : public IConsolePrinter
{
public:
  void Print(const Napi::CallbackInfo& info) override;
  void PrintRaw(const char* str) override;
};
