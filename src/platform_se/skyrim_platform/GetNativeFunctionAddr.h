#pragma once
#include <RE/BSScript/IFunction.h>

class GetNativeFunctionAddr
{
public:
  struct Result
  {
    void* fn = nullptr;
    bool useLongSignature = false;
  };
  static Result Run(const RE::BSScript::IFunction& f);
};