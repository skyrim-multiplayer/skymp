#pragma once
#include "VmCallback.h"
#include "VmFunctionArguments.h"
#include <RE/BSScript/Internal/VirtualMachine.h>
#include <functional>
#include <sstream>

class VmCall
{
public:
  static bool Run(RE::BSScript::IVirtualMachine& vm,
                  RE::BSFixedString className, RE::BSFixedString functionName,
                  const RE::BSTSmartPointer<RE::BSScript::Object>* self,
                  const VmFunctionArguments& arguments,
                  const VmCallback::OnResult& onResult,
                  std::function<const char*()> getExceptionInfo)
  {
    auto args = const_cast<VmFunctionArguments*>(&arguments);
    auto functor = VmCallback::New(onResult);
    if (self && *self)
      return vm.DispatchMethodCall2((*self)->handle, className, functionName,
                                    args, functor);

    std::stringstream err;

    bool res =
      ([](RE::BSScript::IVirtualMachine& vm, RE::BSFixedString& className,
          RE::BSFixedString& functionName, VmFunctionArguments* args,
          RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>& functor,
          std::stringstream& err,
          std::function<const char*()>& getExceptionInfo) {
        __try {
          return vm.DispatchStaticCall(className, functionName, args, functor);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
          if (getExceptionInfo)
            err << getExceptionInfo();
          return false;
        }
      })(vm, className, functionName, args, functor, err, getExceptionInfo);

    if (!err.str().empty())
      throw std::runtime_error("VmCall::Run " + err.str());

    return res;
  }
};
