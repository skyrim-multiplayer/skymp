#include "Sp3Api.h"
#include "CallNative.h"
#include "CallNativeApi.h"
#include "Hooks.h"
#include "NullPointerException.h"
#include "SP3NativeValueCasts.h"
#include "VmProvider.h"
#include <cmrc/cmrc.hpp>
#include <sstream>
#include "PapyrusTESModPlatform.h"

CMRC_DECLARE(skyrim_plugin_resources);

namespace sp3api::internal {
std::function<CallNativeApi::NativeCallRequirements()>
  g_getNativeCallRequirements;

std::shared_ptr<Napi::FunctionReference> g_wrapObjectFunction;

std::string GetSp3Js(const char* pathInAssets)
{
  cmrc::file file;
  try {
    file = cmrc::skyrim_plugin_resources::get_filesystem().open(pathInAssets);
  } catch (std::exception& e) {
    auto dir =
      cmrc::skyrim_plugin_resources::get_filesystem().iterate_directory("");
    std::stringstream ss;
    ss << e.what() << std::endl << std::endl;
    ss << "Root directory contents is: " << std::endl;
    for (auto entry : dir) {
      ss << entry.filename() << std::endl;
    }
    throw std::runtime_error(ss.str());
  }

  auto begin = file.begin();
  auto size = file.size();
  return std::string(begin, begin + size);
}

std::vector<std::string> SP3ListStaticFunctionsImpl(
  const std::string& className)
{
  auto boundNatives = Hooks::GetBoundNatives();
  std::vector<std::string> staticFunctions;

  for (auto [classNameOfFunc, funcName, func] : boundNatives) {
    if (!stricmp(classNameOfFunc.data(), className.data())) {

      auto& vmProvider = VmProvider::GetSingleton();
      FunctionInfo* functionInfo =
        vmProvider.GetFunctionInfo(className, funcName);

      if (functionInfo && functionInfo->IsGlobal()) {
        staticFunctions.push_back(funcName);
      }
    }
  }

  return staticFunctions;
}

std::vector<std::string> SP3ListMethodsImpl(const std::string& className)
{
  std::vector<std::string> methods;
  auto boundNatives = Hooks::GetBoundNatives();

  for (auto [classNameOfFunc, funcName, func] : boundNatives) {
    if (!stricmp(classNameOfFunc.data(), className.data())) {

      auto& vmProvider = VmProvider::GetSingleton();
      FunctionInfo* functionInfo =
        vmProvider.GetFunctionInfo(className, funcName);

      if (functionInfo && !functionInfo->IsGlobal()) {
        methods.push_back(funcName);
      }
    }
  }

  return methods;
}
}

void Sp3Api::Register(Napi::Env env, Napi::Object& exports,
                      std::function<CallNativeApi::NativeCallRequirements()>
                        getNativeCallRequirements)
{
  sp3api::internal::g_getNativeCallRequirements = getNativeCallRequirements;

  Napi::Object sp3backend = exports;
  // Napi::Object::New(env);

  sp3backend.Set(
    "_sp3ListClasses",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3ListClasses)));
  sp3backend.Set(
    "_sp3GetBaseClass",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3GetBaseClass)));
  sp3backend.Set(
    "_sp3ListStaticFunctions",
    Napi::Function::New(
      env, NapiHelper::WrapCppExceptions(SP3ListStaticFunctions)));
  sp3backend.Set(
    "_sp3ListMethods",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3ListMethods)));
  sp3backend.Set(
    "_sp3GetFunctionImplementation",
    Napi::Function::New(
      env, NapiHelper::WrapCppExceptions(SP3GetFunctionImplementation)));
  sp3backend.Set(
    "_sp3DynamicCast",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3DynamicCast)));

  sp3backend.Set("_sp3GetCurrentTickId",
                 Napi::Function::New(
                   env, NapiHelper::WrapCppExceptions(SP3GetCurrentTickId)));

  sp3backend.Set(
    "_sp3RegisterWrapObjectFunction",
    Napi::Function::New(
      env, NapiHelper::WrapCppExceptions(SP3RegisterWrapObjectFunction)));

  {
    constexpr auto kPathInAssets = "assets/sp3.js";
    std::string sp3SourceCode = sp3api::internal::GetSp3Js(kPathInAssets);
    Napi::Value createSkyrimPlatform =
      NapiHelper::RunScript(env, sp3SourceCode);

    createSkyrimPlatform.As<Napi::Function>().Call({ sp3backend, exports });
  }

  {
    constexpr auto kPathInAssets = "assets/storageProxy.js";
    std::string src = sp3api::internal::GetSp3Js(kPathInAssets);
    Napi::Value initStorageProxy = NapiHelper::RunScript(env, src);

    initStorageProxy.As<Napi::Function>().Call({ exports });
  }
}

Napi::Value Sp3Api::SP3ListClasses(const Napi::CallbackInfo& info)
{
  auto boundNatives = Hooks::GetBoundNatives();

  std::set<std::string> classes;
  for (auto [className, funcName, func] : boundNatives) {
    classes.insert(className);
  }

  // Now seek for classes that are not bound to natives like WorldSpace which
  // has zero natives. Searching in argument types and return types of all
  // functions in all scripts.

  auto& provider = VmProvider::GetSingleton();

  std::set<std::string> classesFromScripts;

  for (auto& cl : classes) {
    auto methods = sp3api::internal::SP3ListMethodsImpl(cl);
    auto staticFunctions = sp3api::internal::SP3ListStaticFunctionsImpl(cl);

    for (auto& functionsVector : { methods, staticFunctions }) {
      for (auto& functionName : functionsVector) {
        if (auto info = provider.GetFunctionInfo(cl, functionName)) {
          // Add return type and argument types to the set
          const char* returnType = info->GetReturnType().className;
          if (returnType && strlen(returnType) > 0) {
            classesFromScripts.insert(returnType);
          }

          // Add argument types to the set
          size_t argCount = info->GetParamCount();
          for (size_t i = 0; i < argCount; i++) {
            const char* argType = info->GetParamType(i).className;
            if (argType && strlen(argType) > 0) {
              classesFromScripts.insert(argType);
            }
          }
        }
      }
    }
  }

  for (auto& cl : classesFromScripts) {
    bool hasStricmpEqual = std::any_of(
      classes.begin(), classes.end(), [&cl](const std::string& className) {
        return !stricmp(className.data(), cl.data());
      });
    if (!hasStricmpEqual) {
      classes.insert(cl);
    }
  }

  Napi::Array result = Napi::Array::New(info.Env(), classes.size());
  uint32_t i = 0;
  for (const auto& className : classes) {
    result.Set(i, Napi::String::New(info.Env(), className));
    i++;
  }

  return result;
}

Napi::Value Sp3Api::SP3GetBaseClass(const Napi::CallbackInfo& info)
{
  auto className = NapiHelper::ExtractString(info[0], "className");

  auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
  RE::BSFixedString classNameBs(className.data());

  RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> outTypeInfoPtr;
  bool success = vm->GetScriptObjectType1(classNameBs, outTypeInfoPtr);

  std::string baseClassName;

  RE::BSScript::ObjectTypeInfo* parent = nullptr;

  if (success) {
    parent = outTypeInfoPtr->GetParent();

    if (parent) {
      baseClassName = parent->GetName();
    }
  }

  return Napi::String::New(info.Env(), baseClassName);
}

Napi::Value Sp3Api::SP3ListStaticFunctions(const Napi::CallbackInfo& info)
{
  auto className = NapiHelper::ExtractString(info[0], "className");

  auto staticFunctions =
    sp3api::internal::SP3ListStaticFunctionsImpl(className);

  Napi::Array result = Napi::Array::New(info.Env(), staticFunctions.size());
  uint32_t i = 0;
  for (const auto& staticFunction : staticFunctions) {
    result.Set(i, Napi::String::New(info.Env(), staticFunction));
    i++;
  }

  return result;
}

Napi::Value Sp3Api::SP3ListMethods(const Napi::CallbackInfo& info)
{
  auto className = NapiHelper::ExtractString(info[0], "className");

  auto methods = sp3api::internal::SP3ListMethodsImpl(className);

  Napi::Array result = Napi::Array::New(info.Env(), methods.size());
  uint32_t i = 0;
  for (const auto& method : methods) {
    result.Set(i, Napi::String::New(info.Env(), method));
    i++;
  }

  return result;
}

Napi::Value Sp3Api::SP3GetFunctionImplementation(
  const Napi::CallbackInfo& info)
{
  auto className = NapiHelper::ExtractString(info[1], "className");
  auto functionName = NapiHelper::ExtractString(info[2], "functionName");

  // Proof of concept for fast out-of-PapyrusVM function implementation
  if (!stricmp(className.data(), "Game") &&
      !stricmp(functionName.data(), "getPlayer")) {
    auto functionImplementation = [](const Napi::CallbackInfo& info) {
      CallNative::ObjectPtr obj;
      obj.reset(
        new CallNative::Object("Actor", RE::PlayerCharacter::GetSingleton()));
      auto res = SP3NativeValueCasts::GetSingleton().NativeObjectToJsObject(
        info.Env(), obj);
      res.As<Napi::Object>().Set("_sp3ObjectType", "Actor");
      return res;
    };

    auto functionImplementationWrapped =
      NapiHelper::WrapCppExceptions(functionImplementation);
    return Napi::Function::New(info.Env(), functionImplementationWrapped);
  }

  auto functionImplementation =
    [className, functionName](const Napi::CallbackInfo& info) {
      if (!sp3api::internal::g_getNativeCallRequirements) {
        throw NullPointerException(
          "sp3api::internal::g_getNativeCallRequirements");
      }

      std::vector<Napi::Value> args;
      args.push_back(Napi::String::New(info.Env(), className));
      args.push_back(Napi::String::New(info.Env(), functionName));

      Napi::Value jsThis = info.This();

      // Hack to detect that this arg refers to class not to an object, so
      // it's a static call
      if (jsThis.IsObject()) {
        if (jsThis.As<Napi::Object>()
              .Get(SP3NativeValueCasts::kSkyrimPlatformIndexInPoolProperty)
              .IsUndefined()) {
          jsThis = info.Env().Undefined();
        }
      }

      args.push_back(jsThis);

      for (size_t i = 0; i < info.Length(); i++) {
        args.push_back(info[i]);
      }

      auto jsRes = CallNativeApi::CallNative(
        info.Env(), args, sp3api::internal::g_getNativeCallRequirements);

      // TODO: handle promises here (like on ObjectReference.disable)
      if (jsRes.IsObject()) {
        auto& vmProvider = VmProvider::GetSingleton();
        FunctionInfo* functionInfo =
          vmProvider.GetFunctionInfo(className, functionName);
        if (functionInfo) {
          const char* returnType = functionInfo->GetReturnType().className;
          jsRes.As<Napi::Object>().Set("_sp3ObjectType", returnType);
        } else {
          spdlog::warn("Sp3Api::SP3GetFunctionImplementation - FunctionInfo "
                       "not found for {}.{}",
                       className, functionName);
        }
      }

      return jsRes;
    };

  auto functionImplementationWrapped =
    NapiHelper::WrapCppExceptions(functionImplementation);

  return Napi::Function::New(info.Env(), functionImplementationWrapped);
}

Napi::Value Sp3Api::SP3DynamicCast(const Napi::CallbackInfo& info)
{
  auto& casts = SP3NativeValueCasts::GetSingleton();

  auto form = casts.JsValueToNativeValue(info[0]);
  auto targetType = NapiHelper::ExtractString(info[1], "targetType");

  return casts.NativeValueToJsValue(info.Env(),
                                    CallNative::DynamicCast(targetType, form));
}

Napi::Value Sp3Api::SP3GetCurrentTickId(const Napi::CallbackInfo& info)
{
  uint64_t numPapyrusUpdates = TESModPlatform::GetNumPapyrusUpdates();
  uint64_t numPapyrusUpdatesHigh = numPapyrusUpdates % 0x100000000;
  uint32_t coerced = static_cast<uint32_t>(numPapyrusUpdatesHigh);
  return Napi::Number::New(info.Env(), coerced);
}

Napi::Value Sp3Api::SP3RegisterWrapObjectFunction(
  const Napi::CallbackInfo& info)
{
  auto func = NapiHelper::ExtractFunction(info[0], "fn");

  auto pers = Napi::Persistent(func);

  auto ptr = new Napi::FunctionReference(std::move(pers));

  sp3api::internal::g_wrapObjectFunction.reset(ptr);

  return info.Env().Undefined();
}

std::shared_ptr<Napi::FunctionReference> Sp3Api::GetWrapObjectFunction()
{
  return sp3api::internal::g_wrapObjectFunction;
}
