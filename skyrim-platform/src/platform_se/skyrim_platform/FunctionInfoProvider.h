#pragma once

class FunctionInfo
{
public:
  struct ValueType
  {
    TypeInfo::RawType type;
    const char* className = "";
  };

  virtual ~FunctionInfo() = default;

  virtual size_t GetParamCount() = 0;
  virtual ValueType GetReturnType() = 0;
  virtual bool IsGlobal() = 0;
  virtual bool IsLatent() = 0;
  virtual bool IsNative() = 0;
  virtual RE::BSTSmartPointer<RE::BSScript::IFunction> GetIFunction() = 0;
  virtual bool UsesLongSignature() = 0;
  virtual ValueType GetParamType(size_t i) = 0;
};

class FunctionInfoProvider
{
public:
  virtual ~FunctionInfoProvider() = default;

  virtual FunctionInfo* GetFunctionInfo(const std::string& className,
                                        const std::string& funcName) = 0;

  virtual bool IsDerivedFrom(const char* derivedClassName,
                             const char* baseClassName) = 0;
};
