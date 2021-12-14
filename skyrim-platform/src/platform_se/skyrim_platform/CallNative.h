#pragma once
#include "FunctionInfoProvider.h"
#include "TaskQueue.h"
#include <RE/BSScript/IFunction.h>
#include <RE/BSScript/Internal/VirtualMachine.h>
#include <RE/BSScript/NF_util/NativeFunctionBase.h>
#include <RE/BSScript/ObjectTypeInfo.h>
#include <RE/BSScript/Variable.h>
#include <RE/BSTSmartPointer.h>
#include <memory>
#include <variant>

namespace CallNative {
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
using AnySafe =
  std::variant<ObjectPtr, double, bool, std::string, std::vector<double>,
               std::vector<std::string>, std::vector<bool>,
               std::vector<CallNative::ObjectPtr>>;

template <class T>
static inline size_t GetIndexFor()
{
  static const AnySafe v = T();
  return v.index();
}

RE::BSScript::Variable AnySafeToVariable(const AnySafe& v,
                                         bool treatNumberAsInt);

using LatentCallback = std::function<void(AnySafe)>;

struct Arguments
{
  RE::BSScript::IVirtualMachine* vm;
  RE::VMStackID stackId;
  const std::string& className;
  const std::string& classFunc;
  const ObjectPtr self;
  const AnySafe* args;
  size_t numArgs;
  FunctionInfoProvider& provider;
  Viet::TaskQueue& gameThrQ;
  Viet::TaskQueue& jsThrQ;
  LatentCallback latentCallback;
};

AnySafe CallNativeSafe(Arguments& args);

AnySafe DynamicCast(const std::string& to, const AnySafe& from);
}
