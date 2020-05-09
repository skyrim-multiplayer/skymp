#include "VmProvider.h"
#include "GetNativeFunctionAddr.h"
#include "NullPointerException.h"
#include <RE/BSScript/IFunction.h>
#include <RE/BSScript/Internal/VirtualMachine.h>
#include <optional>

namespace {
const RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo>& FindType(
  const std::string& className)
{
  auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
  if (!vm)
    throw NullPointerException("vm");

  for (int i = 0; i < 2; ++i) {
    for (auto& [thisClassName, classInfo] : vm->objectTypeMap) {
      if (!stricmp(thisClassName.data(), className.data()) != 0)
        return classInfo;
    }
    if (!vm->ReloadType(className.data())) break;
  }

  static const RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> notFound;
  return notFound;
}

struct AdditionalFunctionInfo
{
  enum class Type
  {
    Invalid = -1,
    Global,
    Member
  };
  Type type = Type::Invalid;

  std::vector<RE::BSScript::TypeInfo> paramTypes;
};

using FunctionFindResult =
  std::pair<RE::BSTSmartPointer<RE::BSScript::IFunction>,
            AdditionalFunctionInfo>;

struct FindFunctionCache
{
  std::unordered_map<
    std::string,
    std::unordered_map<std::string, std::unique_ptr<FunctionInfo>>>
    funcByFullName;
};

FunctionFindResult FindFunction(const RE::BSScript::ObjectTypeInfo* classInfo,
                                const std::string& funcName)
{
  FunctionFindResult res;

  for (UInt32 i = 0; i < classInfo->GetNumGlobalFuncs(); ++i) {
    auto& f = classInfo->GetGlobalFuncIter()[i].func;
    if (f && !stricmp(f->GetName().data(), funcName.data())) {
      res.first = f;
      res.second.type = AdditionalFunctionInfo::Type::Global;
      break;
    }
  }
  for (UInt32 i = 0; i < classInfo->GetNumMemberFuncs(); ++i) {
    auto& f = classInfo->GetMemberFuncIter()[i].func;
    if (f && !stricmp(f->GetName().data(), funcName.data())) {
      res.first = f;
      res.second.type = AdditionalFunctionInfo::Type::Member;
      break;
    }
  }

  if (res.first) {
    RE::BSFixedString outNameDummy;
    auto n = res.first->GetParamCount();
    res.second.paramTypes.resize(n);
    for (UInt32 i = 0; i < n; ++i)
      res.first->GetParam(i, outNameDummy, res.second.paramTypes[i]);
  }

  return res;
}

class VmFunctionInfo : public FunctionInfo
{
public:
  VmFunctionInfo(const FunctionFindResult& r_)
    : f(r_.first)
    , info(r_.second)
  {
  }

  size_t GetParamCount() override { return info.paramTypes.size(); }

  ValueType GetReturnType() override
  {
    return MakeValueType(f->GetReturnType());
  }

  bool IsGlobal() override
  {
    switch (info.type) {
      case AdditionalFunctionInfo::Type::Global:
        return true;
      case AdditionalFunctionInfo::Type::Member:
        return false;
      default:
        throw std::runtime_error("Invalid function type");
    }
  }

  bool IsLatent() override { return GetNativeFunctionAddr::Run(*f).isLatent; }

  RE::BSTSmartPointer<RE::BSScript::IFunction> GetIFunction() override
  {
    return f;
  }

  bool UsesLongSignature() override
  {
    return GetNativeFunctionAddr::Run(*f).useLongSignature;
  }

  ValueType GetParamType(size_t i) override
  {
    if (i >= info.paramTypes.size())
      throw std::runtime_error("GetParamType: Bad index (" +
                               std::to_string(i) + "), size was (" +
                               std::to_string(info.paramTypes.size()) + ")");
    return MakeValueType(info.paramTypes[i]);
  }

private:
  RE::BSTSmartPointer<RE::BSScript::IFunction> f;
  AdditionalFunctionInfo info;

  static ValueType MakeValueType(RE::BSScript::TypeInfo& typeInfo)
  {
    ValueType res;
    res.type = typeInfo.GetUnmangledRawType();
    if (res.type == RE::BSScript::TypeInfo::RawType::kObject) {
      auto objTypeInfo = typeInfo.GetTypeInfo();
      if (!objTypeInfo)
        throw NullPointerException("objTypeInfo");
      res.className = objTypeInfo->GetName();
    }
    return res;
  }
};

const std::unique_ptr<FunctionInfo>& FindFunction(FindFunctionCache& cache,
                                                  const std::string& className,
                                                  const std::string& funcName)
{
  auto& f = cache.funcByFullName[className][funcName];
  if (!f) {
    auto& classInfo = FindType(className);
    if (!classInfo)
      throw std::runtime_error("'" + std::string(className) +
                               "' is not a valid Papyrus class name");

    RE::BSScript::ObjectTypeInfo* p = classInfo.get();
    while (p) {
      auto findRes = FindFunction(p, funcName);
      if (findRes.first) {
        f.reset(new VmFunctionInfo(findRes));
        break;
      }
      p = p->GetParent();
    }
  }
  return f;
}

bool IsDerivedFrom(const RE::BSScript::ObjectTypeInfo& obj,
                   const RE::BSScript::ObjectTypeInfo& requestedParamType)
{
  auto p = &obj;
  while (p) {
    if (!stricmp(p->GetName(), requestedParamType.GetName()))
      return true;
    p = obj.GetParent();
  }
  return false;
}
}

struct VmProvider::Impl
{
  std::unordered_map<std::string, std::optional<bool>> isDerivedFromCache;
  FindFunctionCache findFunctionCache;
};

VmProvider::VmProvider()
  : pImpl(new Impl)
{
}

FunctionInfo* VmProvider::GetFunctionInfo(const std::string& className,
                                          const std::string& funcName)
{
  return ::FindFunction(pImpl->findFunctionCache, className, funcName).get();
}

bool VmProvider::IsDerivedFrom(const char* derivedClassName,
                               const char* baseClassName)
{
  char buf[1024] = { 0 };
  sprintf_s(buf, "%s.%s", derivedClassName, baseClassName);
  auto& v = pImpl->isDerivedFromCache[buf];
  if (!v.has_value()) {
    auto derived = FindType(derivedClassName);
    auto base = FindType(baseClassName);
    v = derived && base && ::IsDerivedFrom(*derived, *base);
  }
  return *v;
}
