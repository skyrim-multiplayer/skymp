#pragma once
#include "FunctionInfoProvider.h"
#include <RE/BSScript/IFunction.h>
#include <RE/BSScript/Internal/VirtualMachine.h>
#include <RE/BSScript/NF_util/NativeFunctionBase.h>
#include <RE/BSScript/ObjectTypeInfo.h>
#include <RE/BSTSmartPointer.h>
#include <memory>
#include <variant>

class CallNative
{
public:
  static constexpr size_t g_maxArgs = 12;

  class Object
  {
  public:
    Object(const char* type_, void* obj_)
      : type(type_)
      , obj(obj_)
    {
    }

    const char* GetType() const { return type; }

    void* GetNativeObjectPtr() const { return obj; }

  private:
    const char* const type;
    void* const obj;
  };

  using ObjectPtr = std::shared_ptr<Object>;
  using AnySafe = std::variant<ObjectPtr, double, bool, std::string>;

  static AnySafe CallNativeSafe(RE::BSScript::IVirtualMachine* vm,
                                RE::VMStackID stackId,
                                const std::string& className,
                                const std::string& classFunc,
                                const AnySafe& self, const AnySafe* args,
                                size_t numArgs,
                                FunctionInfoProvider& provider);

  static AnySafe DynamicCast(const std::string& to, const AnySafe& from);
};