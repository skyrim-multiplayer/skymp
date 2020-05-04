#include "CallNative.h"
#include "GetNativeFunctionAddr.h"
#include "NullPointerException.h"
#include "Overloaded.h"
#include <RE/TESObjectREFR.h>

CallNative::Any CallNative::Apply(Any (*nativeFn)(...),
                                  RE::BSScript::IVirtualMachine* vm,
                                  RE::VMStackID stackId, void* self,
                                  const Any* args, size_t numArgs,
                                  bool useLongSignature)
{
  if (useLongSignature) {
    switch (numArgs) {
      case 0:
        return nativeFn(vm, stackId, self);
      case 1:
        return nativeFn(vm, stackId, self, args[0]);
      case 2:
        return nativeFn(vm, stackId, self, args[0], args[1]);
      case 3:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2]);
      case 4:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2], args[3]);
      case 5:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2], args[3],
                        args[4]);
      case 6:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2], args[3],
                        args[4], args[5]);
      case 7:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2], args[3],
                        args[4], args[5], args[6]);
      case 8:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2], args[3],
                        args[4], args[5], args[6], args[7]);
      case 9:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2], args[3],
                        args[4], args[5], args[6], args[7], args[8]);
      case 10:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2], args[3],
                        args[4], args[5], args[6], args[7], args[8], args[9]);
      case 11:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2], args[3],
                        args[4], args[5], args[6], args[7], args[8], args[9],
                        args[10]);
      case 12:
        return nativeFn(vm, stackId, self, args[0], args[1], args[2], args[3],
                        args[4], args[5], args[6], args[7], args[8], args[9],
                        args[10], args[11]);
      default:
        throw std::runtime_error("Bad arg count " + std::to_string(numArgs));
    }
  } else {
    switch (numArgs) {
      case 0:
        return nativeFn(self);
      case 1:
        return nativeFn(self, args[0]);
      case 2:
        return nativeFn(self, args[0], args[1]);
      case 3:
        return nativeFn(self, args[0], args[1], args[2]);
      case 4:
        return nativeFn(self, args[0], args[1], args[2], args[3]);
      case 5:
        return nativeFn(self, args[0], args[1], args[2], args[3], args[4]);
      case 6:
        return nativeFn(self, args[0], args[1], args[2], args[3], args[4],
                        args[5]);
      case 7:
        return nativeFn(self, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6]);
      case 8:
        return nativeFn(self, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7]);
      case 9:
        return nativeFn(self, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7], args[8]);
      case 10:
        return nativeFn(self, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7], args[8], args[9]);
      case 11:
        return nativeFn(self, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7], args[8], args[9], args[10]);
      case 12:
        return nativeFn(self, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7], args[8], args[9], args[10],
                        args[11]);
      default:
        throw std::runtime_error("Bad arg count " + std::to_string(numArgs));
    }
  }
}

CallNative::Any CallNative::CallNativeUnsafe(RE::BSScript::IVirtualMachine* vm,
                                             RE::VMStackID stackId,
                                             void* nativeFn,
                                             bool useLongSignature, void* self,
                                             const Any* args, size_t numArgs)
{
  return Apply(reinterpret_cast<Any (*)(...)>(nativeFn), vm, stackId, self,
               args, numArgs, useLongSignature);
}

template <class T>
inline size_t GetIndexFor()
{
  static const CallNative::AnySafe v = T();
  return v.index();
}

int ToInt(const CallNative::AnySafe& v)
{
  return std::visit(
    overloaded{
      [](double f) { return (int)floor(f); }, [](bool b) { return b ? 1 : 0; },
      [](const std::string& s) { return atoi(s.data()); },
      [](auto) -> int {
        throw std::runtime_error("Unable to cast the argument to Int");
      } },
    v);
}

float ToFloat(const CallNative::AnySafe& v)
{
  return std::visit(
    overloaded{
      [](double f) { return (float)f; }, [](bool b) { return b ? 1.f : 0.f; },
      [](const std::string& s) { return (float)atof(s.data()); },
      [](auto) -> float {
        throw std::runtime_error("Unable to cast the argument to Float");
      } },
    v);
}

bool ToBool(const CallNative::AnySafe& v)
{
  return std::visit(
    overloaded{
      [](double f) { return f >= 0; }, [](bool b) { return b; },
      [](const std::string& s) {
        return s != "0" &&
          s != ""; /*https://dorey.github.io/JavaScript-Equality-Table/*/
      },
      [](const CallNative::ObjectPtr& obj) { return !!obj; },
      [](auto) -> bool {
        throw std::runtime_error("Unable to cast the argument to Bool");
      } },
    v);
}

void* ToObject(const CallNative::AnySafe& v, const char* t,
               FunctionInfoProvider& provider)
{
  return std::visit(
    overloaded{ [&](const CallNative::ObjectPtr& obj) {
                 if (!obj)
                   return (void*)0;
                 auto objPtr = obj->GetNativeObjectPtr();
                 if (!objPtr)
                   throw NullPointerException("objPtr");
                 if (!provider.IsDerivedFrom(obj->GetType(), t)) {
                   std::stringstream ss;
                   ss << obj->GetType()
                      << " is not a valid value for argument with type " << t;
                   throw std::runtime_error(ss.str());
                 }
                 return objPtr;
               },
                [](auto) -> void* {
                  throw std::runtime_error(
                    "Unable to cast the argument to Object");
                } },
    v);
}

std::string ToString(const CallNative::AnySafe& v)
{
  return std::visit(
    overloaded{ [](double f) { return std::to_string(f); },
                [](bool b) { return std::string(b ? "true" : "false"); },
                [](const std::string& s) { return s; },
                [](auto) -> std::string {
                  throw std::runtime_error(
                    "Unable to cast the argument to String");
                } },
    v);
}

CallNative::AnySafe CallNative::CallNativeSafe(
  RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
  const std::string& className, const std::string& classFunc,
  const AnySafe& self, const AnySafe* args, size_t numArgs,
  FunctionInfoProvider& provider)
{
  auto funcInfo = provider.GetFunctionInfo(className, classFunc);
  if (!funcInfo) {
    throw std::runtime_error("Native function not found '" +
                             std::string(className) + "." +
                             std::string(classFunc) + "' ");
  }

  if (self.index() != GetIndexFor<ObjectPtr>())
    throw std::runtime_error("Expected self to be an object");

  auto& selfObjPtr = std::get<ObjectPtr>(self);
  RE::TESForm* rawSelf =
    selfObjPtr ? (RE::TESForm*)selfObjPtr->GetNativeObjectPtr() : nullptr;

  if (rawSelf && funcInfo->IsGlobal()) {
    throw std::runtime_error("Expected self to be null ('" +
                             std::string(className) + "." +
                             std::string(classFunc) + "' is Global function)");
  }
  if (!rawSelf && !funcInfo->IsGlobal()) {
    throw std::runtime_error("Expected self to be non-null ('" +
                             std::string(className) + "." +
                             std::string(classFunc) + "' is Member function)");
  }

  if (numArgs > g_maxArgs)
    throw std::runtime_error("Too many arguments passed (" +
                             std::to_string(numArgs) + "), the limit is " +
                             std::to_string(g_maxArgs));

  if (funcInfo->GetParamCount() != numArgs) {
    std::stringstream ss;
    ss << "Function requires " << funcInfo->GetParamCount()
       << " arguments, but " << numArgs << " passed";
    throw std::runtime_error(ss.str());
  }

  Any argsRaw[g_maxArgs + 1];

  thread_local std::vector<std::string> stringsHolder(g_maxArgs + 1);
  char* stringPtrs[g_maxArgs + 1];

  for (size_t i = 0; i < numArgs; ++i) {
    auto [type, className] = funcInfo->GetParamType(i);
    switch (type) {
      case RE::BSScript::TypeInfo::RawType::kNone:
        throw std::runtime_error(
          "Functions with None arguments are not supported");
      case RE::BSScript::TypeInfo::RawType::kObject: {
        argsRaw[i].obj = ToObject(args[i], className, provider);
        break;
      }
      case RE::BSScript::TypeInfo::RawType::kString: {
        stringsHolder[i] = ToString(args[i]);
        stringPtrs[i] = stringsHolder[i].data();
        argsRaw[i].s = const_cast<const char**>(&stringPtrs[i]);
        break;
      }
      case RE::BSScript::TypeInfo::RawType::kInt:
        argsRaw[i].i = ToInt(args[i]);
        break;
      case RE::BSScript::TypeInfo::RawType::kFloat:
        argsRaw[i].f = ToFloat(args[i]);
        break;
      case RE::BSScript::TypeInfo::RawType::kBool:
        argsRaw[i].b = ToBool(args[i]);
        break;

      case RE::BSScript::TypeInfo::RawType::kNoneArray:
      case RE::BSScript::TypeInfo::RawType::kObjectArray:
      case RE::BSScript::TypeInfo::RawType::kStringArray:
      case RE::BSScript::TypeInfo::RawType::kIntArray:
      case RE::BSScript::TypeInfo::RawType::kFloatArray:
      case RE::BSScript::TypeInfo::RawType::kBoolArray:
        throw std::runtime_error(
          "Functions with Array arguments are not supported");
    }
  }

  CallNative::Any r;

  // Temporary fix for GetPosition* functions
  if ((!stricmp(className.data(), "Actor") ||
       !stricmp(className.data(), "ObjectReference")) &&
      !stricmp(classFunc.data(), "GetPositionX")) {
    r.f = ((RE::TESObjectREFR*)rawSelf)->GetPositionX();
  } else if ((!stricmp(className.data(), "Actor") ||
              !stricmp(className.data(), "ObjectReference")) &&
             !stricmp(classFunc.data(), "GetPositionY")) {
    r.f = ((RE::TESObjectREFR*)rawSelf)->GetPositionY();
  } else if ((!stricmp(className.data(), "Actor") ||
              !stricmp(className.data(), "ObjectReference")) &&
             !stricmp(classFunc.data(), "GetPositionZ")) {
    r.f = ((RE::TESObjectREFR*)rawSelf)->GetPositionZ();
  } else {
    r = CallNativeUnsafe(vm, stackId, funcInfo->GetNativeFunctionAddr(),
                         funcInfo->UsesLongSignature(), rawSelf, argsRaw,
                         numArgs);
  }

  switch (funcInfo->GetReturnType().type) {
    case RE::BSScript::TypeInfo::RawType::kNone:
      return ObjectPtr();
    case RE::BSScript::TypeInfo::RawType::kObject: {
      return std::make_shared<Object>(funcInfo->GetReturnType().className,
                                      r.obj);
    }
    case RE::BSScript::TypeInfo::RawType::kString:
      throw std::runtime_error(
        "Functions with String return type are not supported");
    case RE::BSScript::TypeInfo::RawType::kInt:
      return (double)r.i;
    case RE::BSScript::TypeInfo::RawType::kFloat:
      return (double)r.f;
    case RE::BSScript::TypeInfo::RawType::kBool:
      return r.b;
    case RE::BSScript::TypeInfo::RawType::kNoneArray:
    case RE::BSScript::TypeInfo::RawType::kObjectArray:
    case RE::BSScript::TypeInfo::RawType::kStringArray:
    case RE::BSScript::TypeInfo::RawType::kIntArray:
    case RE::BSScript::TypeInfo::RawType::kFloatArray:
    case RE::BSScript::TypeInfo::RawType::kBoolArray:
      throw std::runtime_error(
        "Functions with Array return type are not supported");
    default:
      throw std::runtime_error("Unknown function return type");
  }
}

// I don't think this implementation covers all cases but it should work
// for TESForm and derived classes
CallNative::AnySafe CallNative::DynamicCast(const std::string& to,
                                            const AnySafe& from_)
{
  static const auto objectIndex = AnySafe(ObjectPtr()).index();
  if (from_.index() != objectIndex)
    throw std::runtime_error("Dynamic cast can only cast objects");

  auto& from = std::get<ObjectPtr>(from_);
  if (!from)
    return ObjectPtr(); // DynamicCast should return None for None argument

  auto rawPtr = from->GetNativeObjectPtr();
  if (!rawPtr)
    throw NullPointerException("rawPtr");

  if (!stricmp(from->GetType(), to.data()))
    return from_;

  if (!stricmp(from->GetType(), "Form") ||
      !stricmp(from->GetType(), "ObjectReference")) {
    auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm)
      throw NullPointerException("vm");

    RE::VMTypeID targetTypeId;
    vm->GetTypeIDForScriptObject(to.data(), targetTypeId);

    auto form = (RE::TESForm*)rawPtr;

    RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> outTypeInfoPtr;
    vm->GetScriptObjectType(targetTypeId, outTypeInfoPtr);
    if (!outTypeInfoPtr)
      throw NullPointerException("outTypeInfoPtr");

    if (targetTypeId == (RE::VMTypeID)form->formType) {
      return std::shared_ptr<Object>(
        new Object(outTypeInfoPtr->GetName(), form));
    }
  }

  return ObjectPtr();
}