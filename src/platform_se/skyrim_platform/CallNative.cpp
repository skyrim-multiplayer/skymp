#include "CallNative.h"
#include "CallNativeApi.h"
#include "GetNativeFunctionAddr.h"
#include "NullPointerException.h"
#include "Overloaded.h"
#include "SendAnimationEvent.h"
#include "StringHolder.h"
#include "VmCall.h"
#include "VmCallback.h"
#include <RE/Actor.h>
#include <RE/BSScript/PackUnpack.h>
#include <RE/BSScript/StackFrame.h>
#include <RE/SkyrimVM.h>
#include <limits>
#include <optional>
#include <skse64/GameReferences.h>
#include <skse64/PapyrusActor.h>
#include <skse64_common/Relocation.h>

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

RE::BSScript::Variable CallNative::AnySafeToVariable(
  const CallNative::AnySafe& v, bool treatNumberAsInt = false)
{
  if (v.valueless_by_exception()) {
    RE::BSScript::Variable res;
    res.SetNone();
    return res;
  }
  return std::visit(
    overloaded{ [&](double f) {
                 RE::BSScript::Variable res;
                 treatNumberAsInt ? res.SetSInt((int)floor(f))
                                  : res.SetFloat((float)f);
                 return res;
               },
                [](bool b) {
                  RE::BSScript::Variable res;
                  res.SetBool(b);
                  return res;
                },
                [](const std::string& s) {
                  RE::BSScript::Variable res;
                  res.SetString(StringHolder::ThreadSingleton()[s]);
                  return res;
                },
                [](const CallNative::ObjectPtr& obj) {
                  RE::BSScript::Variable res;
                  res.SetNone();
                  if (!obj) {
                    return res;
                  }
                  auto nativePtrRaw = (RE::TESForm*)obj->GetNativeObjectPtr();
                  if (!nativePtrRaw)
                    throw NullPointerException("nativePtrRaw");

                  RE::BSScript::PackHandle(
                    &res, nativePtrRaw,
                    static_cast<RE::VMTypeID>(nativePtrRaw->formType));
                  return res;
                },
                [](auto) -> RE::BSScript::Variable {
                  throw std::runtime_error(
                    "Unable to cast the argument to RE::BSScript::Variable");
                } },
    v);
}

class VariableAccess : public RE::BSScript::Variable
{
public:
  VariableAccess() = delete;

  static RE::BSScript::TypeInfo GetType(const RE::BSScript::Variable& v)
  {
    return ((VariableAccess&)v).varType;
  }

  static RE::BSTSmartPointer<RE::BSScript::Object> GetObjectSmartPtr(
    const RE::BSScript::Variable& v)
  {
    return ((VariableAccess&)v).value.obj;
  }
};

namespace {
CallNative::AnySafe VariableToAnySafe(
  const RE::BSScript::Variable& r,
  std::optional<const char*> className = std::nullopt)
{
  using namespace CallNative;

  auto vmImpl = RE::BSScript::Internal::VirtualMachine::GetSingleton();
  if (!vmImpl)
    throw NullPointerException("vmImpl");

  auto t = VariableAccess::GetType(r);
  switch (t.GetUnmangledRawType()) {
    case RE::BSScript::TypeInfo::RawType::kNone:
      return ObjectPtr();
    case RE::BSScript::TypeInfo::RawType::kObject: {
      auto object = r.GetObject();
      if (!object)
        return ObjectPtr();

      auto policy = vmImpl->GetObjectHandlePolicy();

      void* objPtr = nullptr;

      for (int i = 0; i < (int)RE::FormType::Max; ++i)
        if (policy->HandleIsType(i, object->handle)) {
          objPtr = object->Resolve(i);
          break;
        }

      if (objPtr) {
        auto objectTypeInfo = t.GetTypeInfo();
        if (!objectTypeInfo)
          throw NullPointerException("objectTypeInfo");
        return std::make_shared<Object>(
          className.has_value() ? *className : objectTypeInfo->GetName(),
          objPtr);
      } else
        return ObjectPtr();
    }
    case RE::BSScript::TypeInfo::RawType::kString:
      return (std::string)r.GetString().data();
    case RE::BSScript::TypeInfo::RawType::kInt:
      return (double)r.GetSInt();
    case RE::BSScript::TypeInfo::RawType::kFloat:
      return (double)r.GetFloat();
    case RE::BSScript::TypeInfo::RawType::kBool:
      return r.GetBool();
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

bool IsActorOrObjectRefr(const std::string& className)
{
  return !stricmp(className.data(), "Actor") ||
    !stricmp(className.data(), "ObjectReference");
}

bool IsActorOrObjectRefr(RE::FormType formType)
{
  return formType == RE::FormType::ActorCharacter ||
    formType == RE::FormType::Reference;
}

bool IsAddItem(const std::string& className, const std::string& classFunc,
               RE::TESForm* rawSelf)
{
  return IsActorOrObjectRefr(className) &&
    !stricmp(classFunc.data(), "addItem") && rawSelf &&
    IsActorOrObjectRefr(rawSelf->formType);
}

bool IsRemoveItem(const std::string& className, const std::string& classFunc,
                  RE::TESForm* rawSelf)
{
  return IsActorOrObjectRefr(className) &&
    !stricmp(classFunc.data(), "removeItem") && rawSelf &&
    IsActorOrObjectRefr(rawSelf->formType);
}
}

CallNative::AnySafe CallNative::CallNativeSafe(Arguments& args_)
{
  auto& [vm, stackId, className, classFunc, self, args, numArgs, provider,
         gameThrQ, jsThrQ, latentCallback] = args_;

  auto funcInfo = provider.GetFunctionInfo(className, classFunc);

  if (!funcInfo) {
    vm->ReloadType(className.data());
    funcInfo = provider.GetFunctionInfo(className, classFunc);
  }

  if (!funcInfo) {
    throw std::runtime_error("Native function not found '" +
                             std::string(className) + "." +
                             std::string(classFunc) + "' ");
  }

  RE::TESForm* rawSelf = nullptr;
  if (!funcInfo->IsGlobal()) {
    if (self)
      rawSelf = (RE::TESForm*)self->GetNativeObjectPtr();
  }

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

  auto f = funcInfo->GetIFunction();
  auto vmImpl = RE::BSScript::Internal::VirtualMachine::GetSingleton();
  if (!vmImpl)
    throw NullPointerException("vmImpl");

  auto stackIterator = vmImpl->allRunningStacks.find(stackId);
  if (stackIterator == vmImpl->allRunningStacks.end())
    throw std::runtime_error("Bad stackIterator");

  auto it = vmImpl->objectTypeMap.find(f->GetObjectTypeName());
  if (it == vmImpl->objectTypeMap.end())
    throw std::runtime_error("Unable to find owning object type");

  stackIterator->second->top->owningFunction = f;
  stackIterator->second->top->owningObjectType = it->second;
  stackIterator->second->top->self = AnySafeToVariable(self);
  stackIterator->second->top->size = numArgs;

  bool isSendAnimEvent = (!stricmp(className.data(), "Debug") &&
                          !stricmp(classFunc.data(), "sendAnimationEvent"));
  if (isSendAnimEvent) {
    std::vector<AnySafe> _args;
    for (auto it = args; it < args + numArgs; it++)
      _args.push_back(*it);
    gameThrQ.AddTask([=] { SendAnimationEvent::Run(_args); });
    return ObjectPtr();
  }

  bool isQueueNiNodeUpdate = !stricmp(classFunc.data(), "queueNiNodeUpdate");
  if (isQueueNiNodeUpdate) {
    CallNative::ObjectPtr _self = self;
    gameThrQ.AddTask([_self] {
      auto nativeActorPtr = (RE::Actor*)_self->GetNativeObjectPtr();
      if (!nativeActorPtr)
        throw NullPointerException("nativeActorPtr");
      if (nativeActorPtr->formType != RE::FormType::ActorCharacter)
        throw std::runtime_error("QueueNiNodeUpdate must be called on Actor");
      papyrusActor::QueueNiNodeUpdate((Actor*)nativeActorPtr);
    });
    return ObjectPtr();
  }

  if (!stricmp(classFunc.data(), "pushActorAway")) {
    auto targetActor = std::get<ObjectPtr>(args_.args[0]);
    if (!targetActor)
      throw NullPointerException("targetActor");

    auto nativeTargetActor = (RE::Actor*)targetActor->GetNativeObjectPtr();
    if (!nativeTargetActor)
      throw NullPointerException("nativeTargetActor");

    if (nativeTargetActor->formType != RE::FormType::ActorCharacter)
      throw std::runtime_error(
        "nativeTargetActor - unexpected formType (" +
        std::to_string(static_cast<int>(nativeTargetActor->formType)) + ")");

    const auto targetActorId = nativeTargetActor->formID;
    const auto mag = static_cast<float>(std::get<double>(args_.args[1]));

    gameThrQ.AddTask([mag, nativeTargetActor, targetActorId] {
      if (LookupFormByID(targetActorId) !=
          reinterpret_cast<void*>(nativeTargetActor))
        return;
      if (!g_nativeCallRequirements.vm)
        throw NullPointerException("g_nativeCallRequirements.vm");

      typedef void(PushActorAway)(void* vm, RE::VMStackID stackId,
                                  RE::Actor* self, RE::Actor* targetActor,
                                  float magnitude);
      RelocPtr<PushActorAway> pushActorAway(10052416);
      pushActorAway.GetPtr()(g_nativeCallRequirements.vm,
                             g_nativeCallRequirements.stackId,
                             nativeTargetActor, nativeTargetActor, mag);
    });
    return ObjectPtr();
  }

  if (!stricmp(classFunc.data(), "getFormID"))
    return (double)rawSelf->formID;

  if (!stricmp(className.data(), "Game") &&
      !stricmp(classFunc.data(), "getFormEx")) {
    auto form =
      RE::TESForm::LookupByID((uint32_t)std::get<double>(args_.args[0]));
    return form ? std::make_shared<Object>("Form", form) : ObjectPtr();
  }

  if (IsAddItem(className, classFunc, rawSelf)) {

    if (auto actor = reinterpret_cast<RE::Actor*>(rawSelf)) {

      auto obj = std::get<CallNative::ObjectPtr>(args_.args[0]);
      int32_t count = std::get<double>(args_.args[1]);

      RE::TESBoundObject* boundObject = obj
        ? reinterpret_cast<RE::TESBoundObject*>(obj->GetNativeObjectPtr())
        : nullptr;

      if (boundObject)
        actor->AddObjectToContainer(boundObject, nullptr, count, nullptr);
    }
    return ObjectPtr();
  }

  if (IsRemoveItem(className, classFunc, rawSelf)) {

    if (auto actor = reinterpret_cast<RE::Actor*>(rawSelf)) {

      auto obj = std::get<CallNative::ObjectPtr>(args_.args[0]);
      int32_t count = std::get<double>(args_.args[1]);
      auto objToMove = std::get<CallNative::ObjectPtr>(args_.args[3]);

      RE::TESBoundObject* boundObject = obj
        ? reinterpret_cast<RE::TESBoundObject*>(obj->GetNativeObjectPtr())
        : nullptr;

      RE::TESObjectREFR* refrToMove = objToMove
        ? reinterpret_cast<RE::TESObjectREFR*>(objToMove->GetNativeObjectPtr())
        : nullptr;

      if (count < 0)
        count = std::numeric_limits<int32_t>::max();

      if (boundObject)
        actor->RemoveItem(boundObject, count, RE::ITEM_REMOVE_REASON::kRemove,
                          nullptr, refrToMove);
    }
    return ObjectPtr();
  }

  auto topArgs = stackIterator->second->top->args;
  for (int i = 0; i < numArgs; i++) {
    RE::BSFixedString unusedNameOut;
    RE::BSScript::TypeInfo typeOut;
    f->GetParam(i, unusedNameOut, typeOut);

    topArgs[i] = AnySafeToVariable(args[i], typeOut.IsInt());
  }

  if (funcInfo->IsLatent()) {
    VmFunctionArguments vmFuncArgs(
      [](void* numArgs) { return (size_t)numArgs; },
      [&args_, &f](size_t i) {
        RE::BSFixedString unusedNameOut;
        RE::BSScript::TypeInfo typeOut;
        f->GetParam(i, unusedNameOut, typeOut);
        return AnySafeToVariable(args_.args[i], typeOut.IsInt());
      },
      (void*)numArgs);
    auto fsClassName = AnySafeToVariable(className).GetString();
    auto fsClassFunc = AnySafeToVariable(classFunc).GetString();
    auto selfScriptObject = rawSelf
      ? VariableAccess::GetObjectSmartPtr(AnySafeToVariable(self))
      : RE::BSTSmartPointer<RE::BSScript::Object>();

    auto funcReturnType = funcInfo->GetReturnType().className;
    auto jsThrQPtr = &jsThrQ;
    auto cb = latentCallback;
    auto onResult = [cb, funcReturnType,
                     jsThrQPtr](const RE::BSScript::Variable& result) {
      jsThrQPtr->AddTask([=] {
        if (!cb)
          throw NullPointerException("cb");
        cb(VariableToAnySafe(result, funcReturnType));
      });
    };
    VmCall::Run(*vmImpl, fsClassName, fsClassFunc, &selfScriptObject,
                vmFuncArgs, onResult, nullptr);
    return ObjectPtr();
  }

  RE::BSScript::IFunction::CallResult callResut =
    f->Call(stackIterator->second, vmImpl->GetErrorLogger(), vmImpl, false);
  if (callResut != RE::BSScript::IFunction::CallResult::kCompleted) {
    throw std::runtime_error("Bad call result " +
                             std::to_string((int)callResut));
  }

  auto& r = stackIterator->second->returnValue;

  return VariableToAnySafe(r, funcInfo->GetReturnType().className);
}

namespace {
bool IsInstanceOf(
  RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo>& objectInfo,
  const char* parentType)
{
  RE::BSScript::ObjectTypeInfo* objectInQuestion = objectInfo.get();

  if (!objectInQuestion)
    throw NullPointerException("objectInQuestion");

  while (true) {

    if (!stricmp(objectInQuestion->GetName(), parentType))
      return true;

    objectInQuestion = objectInQuestion->GetParent();

    if (!objectInQuestion)
      return false;
  }
};

void GetScriptObjectType(
  RE::BSScript::Internal::VirtualMachine& vm, RE::VMTypeID vmTypeID,
  RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo>& objectTypeInfo)
{
  bool res = vm.GetScriptObjectType(vmTypeID, objectTypeInfo);

  if (!objectTypeInfo)
    throw NullPointerException("objectTypeInfo");

  if (!res)
    throw std::runtime_error("GetScriptObjectType returned false");
};

}

// I don't think this implementation covers all cases but it should work
// for TESForm and derived classes
CallNative::AnySafe CallNative::DynamicCast(const std::string& outputTypeName,
                                            const AnySafe& from_)
{
  static const auto objectIndex = AnySafe(ObjectPtr()).index();
  if (from_.index() != objectIndex)
    throw std::runtime_error("Dynamic cast can only cast objects");

  auto& fromObjPtr = std::get<ObjectPtr>(from_);
  if (!fromObjPtr)
    return ObjectPtr(); // DynamicCast should return None for None argument

  auto rawPtr = fromObjPtr->GetNativeObjectPtr();
  if (!rawPtr)
    throw NullPointerException("rawPtr");

  if (!stricmp(fromObjPtr->GetType(), outputTypeName.data()))
    return from_;

  auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
  if (!vm)
    throw NullPointerException("vm");

  RE::VMTypeID fromTypeId;
  RE::VMTypeID outputTypeId;

  if (!vm->GetTypeIDForScriptObject(fromObjPtr->GetType(), fromTypeId))
    return ObjectPtr();

  if (!vm->GetTypeIDForScriptObject(outputTypeName.data(), outputTypeId))
    return ObjectPtr();

  RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> fromTypeInfoPtr;
  RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> outTypeInfoPtr;

  GetScriptObjectType(*vm, fromTypeId, fromTypeInfoPtr);
  GetScriptObjectType(*vm, outputTypeId, outTypeInfoPtr);

  if (IsInstanceOf(fromTypeInfoPtr, "Form")) {

    auto form = (RE::TESForm*)rawPtr;

    RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> objInfoFormType;
    RE::VMTypeID formTypeId = (RE::VMTypeID)form->formType;
    GetScriptObjectType(*vm, formTypeId, objInfoFormType);

    const bool IsParentOf =
      IsInstanceOf(outTypeInfoPtr, fromTypeInfoPtr->GetName());
    const bool IsChildOf =
      IsInstanceOf(fromTypeInfoPtr, outTypeInfoPtr->GetName());

    const bool IsFormHasNeedParent =
      IsInstanceOf(objInfoFormType, outputTypeName.data());

    if (IsChildOf || IsParentOf && IsFormHasNeedParent) {

      return std::shared_ptr<Object>(
        new Object(outTypeInfoPtr->GetName(), form));
    }
  }

  return ObjectPtr();
}