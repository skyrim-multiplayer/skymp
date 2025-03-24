#pragma once
#include "SpSnippetFunctionGen.h"
#include "papyrus-vm/VirtualMachine.h"
#include "script_compatibility_policies/IPapyrusCompatibilityPolicy.h"

class IPapyrusClassBase
{
public:
  virtual const char* GetName() = 0;

  virtual void Register(
    VirtualMachine& vm,
    std::shared_ptr<IPapyrusCompatibilityPolicy> policy) = 0;

  virtual ~IPapyrusClassBase() = default;

  static VarValue ExecuteSpSnippetAndGetPromise(
    const char* script, const char* name,
    std::shared_ptr<IPapyrusCompatibilityPolicy> policy, VarValue self,
    const std::vector<VarValue>& arguments, bool method = false,
    SpSnippetMode mode = SpSnippetMode::kNoReturnResult,
    const VarValue& defaultResult = VarValue::None());

public:
  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};

template <class T>
class IPapyrusClass : public IPapyrusClassBase
{
public:
  template <class MemberFn>
  void AddStatic(VirtualMachine& vm, const char* funcName, MemberFn memberFn)
  {
    auto this_ = dynamic_cast<T*>(this);
    vm.RegisterFunction(
      GetName(), funcName, FunctionType::GlobalFunction,
      [this_, memberFn](VarValue self, const std::vector<VarValue>& arg)
        -> VarValue { return (this_->*memberFn)(self, arg); });
  }

  template <class MemberFn>
  void AddMethod(VirtualMachine& vm, const char* funcName, MemberFn memberFn)
  {
    auto this_ = dynamic_cast<T*>(this);
    vm.RegisterFunction(
      GetName(), funcName, FunctionType::Method,
      [this_, memberFn](VarValue self, const std::vector<VarValue>& arg)
        -> VarValue { return (this_->*memberFn)(self, arg); });
  }
};
