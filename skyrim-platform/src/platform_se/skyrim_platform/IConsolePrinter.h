#pragma once

class JsFunctionArguments;

class IConsolePrinter
{
public:
  virtual ~IConsolePrinter() = default;

  virtual void Print(const JsFunctionArguments& args) = 0;
};
