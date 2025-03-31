#pragma once

class IConsolePrinter
{
public:
  virtual ~IConsolePrinter() = default;

  virtual void Print(const Napi::CallbackInfo& info) = 0;
  virtual void PrintRaw(const char* str) = 0;
};
