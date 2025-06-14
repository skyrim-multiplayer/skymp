#pragma once
#include "Hooks.h"

class DumpFunctions
{
public:
  static void Run();

private:
  static void RunImpl(
    const std::vector<
      std::tuple<std::string, std::string, RE::BSScript::IFunction*>>& data);
};
