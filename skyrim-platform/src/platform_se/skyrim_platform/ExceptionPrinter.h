#pragma once

class ExceptionPrinter
{
public:
  ExceptionPrinter(const char* exceptionPrefix_);

  void PrintException(const char* what);

private:
  const char* const exceptionPrefix;
};
