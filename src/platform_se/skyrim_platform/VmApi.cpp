#include "VmApi.h"

#include "InvalidArgumentException.h"
#include "NullPointerException.h"
#include "VmCall.h"
#include <RE/BSScript/Array.h>
#include <RE/BSScript/IFunction.h>
#include <RE/BSScript/Internal/VirtualMachine.h>
#include <RE/BSScript/ObjectTypeInfo.h>
#include <RE/SkyrimVM.h>
#include <algorithm>
#include <unordered_map>
#include <RE/TESObjectREFR.h>
#include <RE/BSScript/IVirtualMachine.h>

#include <RE/ConsoleLog.h>

extern TaskQueue* VmApi::taskQueue = nullptr;

using TypeInfoPtr = RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo>;

struct VmCache
{
  std::vector<TypeInfoPtr> types;
};

struct VmStringCache
{
  std::unordered_map<std::string, std::shared_ptr<RE::BSFixedString>> data;
  std::vector<std::unique_ptr<std::string>> stringsHolder;

  RE::BSFixedString* operator[](const std::string& s)
  {
    auto& r = data[s];
    if (!r) {
      stringsHolder.push_back(std::make_unique<std::string>(s));
      r.reset(new RE::BSFixedString(stringsHolder.back()->c_str()));
    }
    return &*r;
  }
};

namespace {
enum class FunctionProp
{
  IsNative = 0,
  IsStatic = 1,
  IsEmpty = 2,
  IsGetter = 3,
  IsSetter = 4,
  UserFlags = 5,
  DocString = 6,
  SourceFileName = 7,
  Arguments = 8,
  ReturnType = 9,
  StateName = 10,
  ObjectTypeName = 11,
  Name = 12
};

void PrepareVmCache(VmCache& cache)
{
  if (!cache.types.empty())
    return;

  auto vmInternal = RE::BSScript::Internal::VirtualMachine::GetSingleton();
  if (!vmInternal)
    throw NullPointerException("vmInternal");

  cache.types.reserve(vmInternal->objectTypeMap.size());
  for (auto& [name, typeInfo] : vmInternal->objectTypeMap)
    cache.types.push_back(typeInfo);

  std::sort(cache.types.begin(), cache.types.end(),
            [&](const TypeInfoPtr& lhs, const TypeInfoPtr& rhs) {
              return (std::string)lhs->GetName() < rhs->GetName();
            });
}

RE::BSScript::ObjectTypeInfo* TypeAt(VmCache& cache, int i)
{
  if (i < 0 || i >= cache.types.size())
    throw InvalidArgumentException("type", i);

  auto typeInfo = cache.types[i].get();
  if (!typeInfo)
    throw NullPointerException("typeInfo");

  return typeInfo;
}

RE::BSScript::ObjectTypeInfo::NamedStateInfo* StateAt(VmCache& cache, int type,
                                                      int state)
{
  auto typeInfo = TypeAt(cache, type);
  int i = state - 1;
  int numStates = (int)typeInfo->GetNumNamedStates();
  if (i < 0 || i >= numStates)
    throw InvalidArgumentException("state", state);

  return &typeInfo->GetNamedStateIter()[i];
}

const char* GetTypeName(const RE::BSScript::TypeInfo& type)
{
  switch (type.GetUnmangledRawType()) {
    case RE::BSScript::TypeInfo::RawType::kNone:
      return "None";
    case RE::BSScript::TypeInfo::RawType::kObject:
      return type.GetTypeInfo() ? type.GetTypeInfo()->GetName() : "Object";
    case RE::BSScript::TypeInfo::RawType::kString:
      return "String";
    case RE::BSScript::TypeInfo::RawType::kInt:
      return "Int";
    case RE::BSScript::TypeInfo::RawType::kFloat:
      return "Float";
    case RE::BSScript::TypeInfo::RawType::kBool:
      return "Bool";
    case RE::BSScript::TypeInfo::RawType::kNoneArray:
      return "NoneArray";
    case RE::BSScript::TypeInfo::RawType::kObjectArray:
      return "ObjectArray";
    case RE::BSScript::TypeInfo::RawType::kStringArray:
      return "StringArray";
    case RE::BSScript::TypeInfo::RawType::kIntArray:
      return "IntArray";
    case RE::BSScript::TypeInfo::RawType::kFloatArray:
      return "FloatArray";
    case RE::BSScript::TypeInfo::RawType::kBoolArray:
      return "BoolArray";
  }
  return "";
}

JsValue GetFunctionProperty(VmCache& cache, RE::BSScript::IFunction* funcInfo,
                            FunctionProp prop)
{
  if (!funcInfo)
    throw NullPointerException("funcInfo");

  switch (prop) {
    case FunctionProp::IsNative:
      return funcInfo->GetIsNative();
    case FunctionProp::IsStatic:
      return funcInfo->GetIsStatic();
    case FunctionProp::IsEmpty:
      return funcInfo->GetIsEmpty();
    case FunctionProp::IsGetter:
      return funcInfo->GetFunctionType() ==
        RE::BSScript::IFunction::FunctionType::kGetter;
    case FunctionProp::IsSetter:
      return funcInfo->GetFunctionType() ==
        RE::BSScript::IFunction::FunctionType::kSetter;
    case FunctionProp::UserFlags:
      return (int)funcInfo->GetUserFlags();
    case FunctionProp::DocString:
      return funcInfo->GetDocString().data();
    case FunctionProp::SourceFileName:
      return funcInfo->GetSourceFilename().data();
    case FunctionProp::Arguments: {
      auto arr = JsValue::Array(funcInfo->GetParamCount());
      for (int i = 0; i < (int)funcInfo->GetParamCount(); ++i) {
        RE::BSFixedString name;
        RE::BSScript::TypeInfo type;
        funcInfo->GetParam((UInt32)i, name, type);

        auto entry = JsValue::Object();
        entry.SetProperty("name", name.data());
        entry.SetProperty("typeName", GetTypeName(type));
        arr.SetProperty(i, entry);
      }
      return arr;
    }
    case FunctionProp::ReturnType:
      return GetTypeName(funcInfo->GetReturnType());
    case FunctionProp::StateName:
      return funcInfo->GetStateName().c_str();
    case FunctionProp::ObjectTypeName:
      return funcInfo->GetObjectTypeName().c_str();
    case FunctionProp::Name:
      return funcInfo->GetName().c_str();
    default:
      throw InvalidArgumentException("prop", (int)prop);
  }
  return JsValue::Undefined();
}

class VariableAccess : public RE::BSScript::Variable
{
public:
  const RE::BSScript::TypeInfo& GetType() const noexcept
  {
    return this->varType;
  }

  const RE::BSTSmartPointer<RE::BSScript::Object>& GetObjectPtr() const
    noexcept
  {
    return this->value.obj;
  }

  void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>& obj)
  {
    this->value.obj = obj;
  }
};

class ExternalObject
{
public:
  virtual ~ExternalObject() = default;
};

class ObjectWrapper : public ExternalObject
{
public:
  ObjectWrapper(const RE::BSTSmartPointer<RE::BSScript::Object>& object_)
    : object(object_)
  {
    if (!object_)
      throw NullPointerException("object_");
    object->IncRef();
  }

  static void Finalize(void* objectWrapper)
  {
    delete reinterpret_cast<ObjectWrapper*>(objectWrapper);
  }

  auto& GetObject() const { return object; }

  ~ObjectWrapper() { object->DecRef(); }

  ObjectWrapper(const ObjectWrapper&) = delete;
  ObjectWrapper& operator=(const ObjectWrapper&) = delete;

private:
  RE::BSTSmartPointer<RE::BSScript::Object> object;
};
JsValue ToString(const JsFunctionArguments& args)
{
  const RE::BSScript::Object* selfPtr = nullptr;
 
  if (args[0].GetType() == JsValue::Type::Object) {
    if (auto externalRaw = args[0].GetExternalData()) {
      auto objWrapper = dynamic_cast<ObjectWrapper*>(
        reinterpret_cast<ExternalObject*>(externalRaw));
      if (!objWrapper)
        throw InvalidArgumentException("self", "<not a papyrus object>");

      selfPtr = objWrapper->GetObject().get();
      if (!selfPtr)
        throw InvalidArgumentException("self", "<null pointer>");

     auto typePtr = selfPtr->GetTypeInfo();
     std::string typeName = typePtr ? typePtr->GetName() : "None";

      return "[Object " + typeName +"] "; 
    }
  }
}

JsValue PapyrusValueToJsValue(const RE::BSScript::Variable& v_)
{
  auto& v = reinterpret_cast<const VariableAccess&>(v_);
  switch (v.GetType().GetUnmangledRawType()) {
    case RE::BSScript::TypeInfo::RawType::kNone:
      return JsValue::Null();
    case RE::BSScript::TypeInfo::RawType::kObject: {
      auto obj = JsValue::ExternalObject(new ObjectWrapper(v.GetObjectPtr()),
                              ObjectWrapper::Finalize);
      thread_local JsValue toString = JsValue::Function(ToString);
      obj.SetProperty("toString", toString);
      return obj;
    }
    case RE::BSScript::TypeInfo::RawType::kString:
      return v_.GetString().data();
    case RE::BSScript::TypeInfo::RawType::kInt:
      return v_.GetSInt();
    case RE::BSScript::TypeInfo::RawType::kFloat:
      return v_.GetFloat();
    case RE::BSScript::TypeInfo::RawType::kBool:
      return JsValue::Bool(v_.GetBool());
    case RE::BSScript::TypeInfo::RawType::kNoneArray:
    case RE::BSScript::TypeInfo::RawType::kObjectArray:
    case RE::BSScript::TypeInfo::RawType::kStringArray:
    case RE::BSScript::TypeInfo::RawType::kIntArray:
    case RE::BSScript::TypeInfo::RawType::kFloatArray:
    case RE::BSScript::TypeInfo::RawType::kBoolArray:
      if (auto papyrusArray = v.GetArray()) {
        auto arr = JsValue::Array(papyrusArray->size());
        for (int i = 0; i < (int)papyrusArray->size(); ++i)
          arr.SetProperty(i, PapyrusValueToJsValue(papyrusArray->at(i)));
        return arr;
      }
      throw NullPointerException("papyrusArray");
  }
  return JsValue::Null();
}

RE::BSScript::Variable JsValueToPapyrusValue(const JsValue& v,
                                             VmStringCache& stringCache)
{
  RE::BSScript::Variable res;

  switch (v.GetType()) {
    case JsValue::Type::Undefined:
    case JsValue::Type::Null:
      res.SetNone();
      break;
    case JsValue::Type::Number: {
      double intpart;
      auto d = (double)v;
      modf(d, &intpart) == 0.0 ? res.SetSInt((int)v) : res.SetFloat((float)d);
      break;
    }
    case JsValue::Type::String:
      res.SetString(*stringCache[(std::string)v]);
      break;
    case JsValue::Type::Boolean:
      res.SetBool((bool)v);
      break;
    case JsValue::Type::Object:
      res.SetNone();
      if (auto external = v.GetExternalData()) {
        auto externalConcrete = reinterpret_cast<ExternalObject*>(external);
        if (auto objWrapper = dynamic_cast<ObjectWrapper*>(externalConcrete)) {
          reinterpret_cast<VariableAccess&>(res).SetObject(
            objWrapper->GetObject());
        }
      }
      break;
    default:
      std::stringstream ss;
      ss << "JS type " << (int)v.GetType()
         << " can't be casted to Papyrus type";
      throw std::runtime_error(ss.str());
  }

  return res;
}

}

namespace VmApi {
static VmCache vmCache;
static VmStringCache vmStringCache;
}

JsValue VmApi::GetTypesCount(const JsFunctionArguments& args)
{
  PrepareVmCache(vmCache);
  return (int)vmCache.types.size();
}

JsValue VmApi::GetTypeName(const JsFunctionArguments& args)
{
  PrepareVmCache(vmCache);
  return TypeAt(vmCache, (int)args[1])->GetName();
}

JsValue VmApi::GetTypeStatesCount(const JsFunctionArguments& args)
{
  PrepareVmCache(vmCache);
  int numNamedStates = (int)TypeAt(vmCache, (int)args[1])->GetNumNamedStates();
  return numNamedStates + 1;
}

JsValue VmApi::GetTypeFunctionsCount(const JsFunctionArguments& args)
{
  PrepareVmCache(vmCache);
  int type = (int)args[1];
  return (int)TypeAt(vmCache, type)->GetNumGlobalFuncs();
}

JsValue VmApi::GetTypeMethodsCount(const JsFunctionArguments& args)
{
  PrepareVmCache(vmCache);
  int type = (int)args[1], state = (int)args[2];

  return state == 0 ? (int)TypeAt(vmCache, type)->GetNumMemberFuncs()
                    : (int)StateAt(vmCache, type, state)->GetNumFuncs();
}

JsValue VmApi::GetMethodProperty(const JsFunctionArguments& args)
{
  PrepareVmCache(vmCache);
  int type = (int)args[1], state = (int)args[2], method = (int)args[3];
  auto prop = static_cast<FunctionProp>((int)args[4]);

  if (state != 0) {
    auto stateInfo = StateAt(vmCache, type, state);

    if (method < 0 || method >= stateInfo->GetNumFuncs())
      throw InvalidArgumentException("method", method);

    auto f = stateInfo->GetFuncIter()[method].func.get();
    return ::GetFunctionProperty(vmCache, f, prop);
  } else {
    auto typeInfo = TypeAt(vmCache, type);

    if (method < 0 || method >= typeInfo->GetNumMemberFuncs())
      throw InvalidArgumentException("method", method);

    auto f = typeInfo->GetMemberFuncIter()[method].func.get();
    return ::GetFunctionProperty(vmCache, f, prop);
  }
}

JsValue VmApi::GetFunctionProperty(const JsFunctionArguments& args)
{
  PrepareVmCache(vmCache);
  int type = (int)args[1], func = (int)args[2];
  auto prop = static_cast<FunctionProp>((int)args[3]);

  auto typeInfo = TypeAt(vmCache, type);
  if (func < 0 || func >= typeInfo->GetNumGlobalFuncs())
    throw InvalidArgumentException("func", func);

  auto funcInfo = typeInfo->GetGlobalFuncIter()[func].func.get();
  if (!funcInfo)
    throw NullPointerException("funcInfo");

  return ::GetFunctionProperty(vmCache, funcInfo, prop);
}

JsValue VmApi::Call(const JsFunctionArguments& args)
{
  auto skyrimVm = RE::SkyrimVM::GetSingleton();
  if (!skyrimVm)
    throw NullPointerException("skyrimVm");
  if (!skyrimVm->impl)
    throw NullPointerException("skyrimVm->impl");

  auto className = (std::string)args[1], funcName = (std::string)args[2];
  auto callback = args.GetSize() > 3 ? args[3] : JsValue::Undefined();
  auto self = args.GetSize() > 4 ? args[4] : JsValue::Undefined();
  static const int numArgsBeforePapyrusArgs =
    5; // this, className, funcName, self, callback

  RE::BSFixedString classNameFs(className.data()), funcNameFs(funcName.data());

  auto getNumArgs = [](void* state) -> size_t {
    auto& args = *reinterpret_cast<JsFunctionArguments*>(state);
    return std::max(0, (int)args.GetSize() - numArgsBeforePapyrusArgs);
  };

  auto getNthArg = [&className, &funcName,
                    &args](size_t i) -> RE::BSScript::Variable {
    auto& arg = args[i + numArgsBeforePapyrusArgs];

    bool treatNumberAsInteger = false;

    // throw std::runtime_error(std::to_string((int)arg.GetType()));

    return JsValueToPapyrusValue(arg, vmStringCache);
  };

  VmFunctionArguments vmFuncArgs(getNumArgs, getNthArg,
                                 const_cast<JsFunctionArguments*>(&args));

  // JsValue ctor/dtor must always be called from thread with active Chakra
  // context, but VmCallback::OnResult is called from the different thread.
  auto callbackPtr = callback.GetType() == JsValue::Type::Function
    ? std::make_shared<JsValue>(callback)
    : nullptr;

  VmCallback::OnResult onResult = [callbackPtr](RE::BSScript::Variable v) {
    if (!taskQueue || !callbackPtr)
      return;
    taskQueue->AddTask([=] {
      try {
        callbackPtr->Call({ PapyrusValueToJsValue(v) });
      } catch (std::exception& e) {
        if (auto console = RE::ConsoleLog::GetSingleton()) {
          console->Print("[Exception] %s", e.what());
        }
      }
    });
  };

  const RE::BSTSmartPointer<RE::BSScript::Object>* selfPtr = nullptr;
  if (self.GetType() == JsValue::Type::Object) {
    if (auto externalRaw = self.GetExternalData()) {
      auto objWrapper = dynamic_cast<ObjectWrapper*>(
        reinterpret_cast<ExternalObject*>(externalRaw));
      if (!objWrapper)
        throw InvalidArgumentException("self", "<not a papyrus object>");
      selfPtr = &objWrapper->GetObject();
    }
  }

  thread_local char exceptionInfoBuf[1024] = { 0 };

  auto getExceptionInfo = [&] {
    std::string args;
    sprintf_s(exceptionInfoBuf, "CallStatic failed for %s.%s(%s)",
              className.data(), funcName.data(), args.data());
    return exceptionInfoBuf;
  };

  if (auto scriptObjPtr = selfPtr->get(); funcName == "SendAnimationEvent") {
    const RE::BSFixedString animEventName = RE::BSFixedString("SendAnimationEvent");
    
    //RE::BSScript::IVirtualMachine::SendEvent(scriptObjPtr->handle,
    //                                         animEventName, &vmFuncArgs);
    
    
    return JsValue::Undefined();
  } 

  VmCall::Run(*skyrimVm->impl, classNameFs, funcName.data(), selfPtr,
              vmFuncArgs, onResult, getExceptionInfo);

  return JsValue::Undefined();
}
