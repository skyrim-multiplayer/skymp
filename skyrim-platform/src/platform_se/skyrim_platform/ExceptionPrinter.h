#pragma once

class ExceptionPrinter
{
public:
  static const char* RemoveMultiplePrefixes(const char* str,
                                            const char* prefix);
  static void Print(const std::exception& e);

private:
  ExceptionPrinter(const char* exceptionPrefix_);
  void PrintException(const char* what);
  const char* const exceptionPrefix;
};
