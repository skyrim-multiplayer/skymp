#pragma once
#include "FunctionInfoProvider.h"

class VmProvider : public FunctionInfoProvider
{
public:
  VmProvider();

  // Must also search in base classes
  FunctionInfo* GetFunctionInfo(const std::string& className,
                                const std::string& funcName) override;

  bool IsDerivedFrom(const char* derivedClassName,
                     const char* baseClassName) override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
