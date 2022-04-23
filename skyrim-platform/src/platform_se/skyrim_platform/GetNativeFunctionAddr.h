#pragma once

class GetNativeFunctionAddr
{
public:
  struct Result
  {
    void* fn = nullptr;
    bool useLongSignature = false;
    bool isLatent = false;
    bool isNative = false;
  };
  static Result Run(const RE::BSScript::IFunction& f);
};
