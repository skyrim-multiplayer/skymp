#include "Sp3Api.h"
#include "Hooks.h"

void Sp3Api::Register(Napi::Env env, Napi::Object& exports)
{
  exports.Set("_sp3ListClasses", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3ListClasses)));
  exports.Set("_sp3GetBaseClass", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3GetBaseClass)));
  exports.Set("_sp3ListStaticFunctions", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3ListStaticFunctions)));
  exports.Set("_sp3ListMethods", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3ListMethods)));
  exports.Set("_sp3GetFunctionImplementation", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3GetFunctionImplementation)));
  exports.Set("_sp3DynamicCast", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SP3DynamicCast)));
}

Napi::Value Sp3Api::SP3ListClasses(const Napi::CallbackInfo& info)
{
  std::vector<RE::BSScript::IFunction *> boundNatives = Hooks::GetBoundNatives();

  std::set<std::string> classes;
  for (auto func : boundNatives) {
    classes.insert(func->GetObjectTypeName().data());
  }

  // TODO: normalize class names on the JS side or here

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

  // TODO: implement

  return info.Env().Undefined();
}

Napi::Value Sp3Api::SP3ListStaticFunctions(const Napi::CallbackInfo& info)
{
  auto className = NapiHelper::ExtractString(info[0], "className");

  std::vector<RE::BSScript::IFunction *> boundNatives = Hooks::GetBoundNatives();

  std::vector<std::string> staticFunctions;

  for (auto func : boundNatives) {
    if (!stricmp(func->GetObjectTypeName().data(), className.data())) {
      if (func->IsStatic()) {
        staticFunctions.push_back(func->GetName().data());
      }
    }
  }

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

  std::vector<RE::BSScript::IFunction *> boundNatives = Hooks::GetBoundNatives();

  std::vector<std::string> methods;

  for (auto func : boundNatives) {
    if (!stricmp(func->GetObjectTypeName().data(), className.data())) {
      if (!func->IsStatic()) {
        methods.push_back(func->GetName().data());
      }
    }
  }

  Napi::Array result = Napi::Array::New(info.Env(), methods.size());
  uint32_t i = 0;
  for (const auto& method : methods) {
    result.Set(i, Napi::String::New(info.Env(), staticFunction));
    i++;
  }

  return result;
}

Napi::Value Sp3Api::SP3GetFunctionImplementation(const Napi::CallbackInfo& info)
{
auto className = NapiHelper::ExtractString(info[1], "className");
    auto functionName = NapiHelper::ExtractString(info[2], "functionName");

    // TODO: implement
  return info.Env().Undefined();
}

Napi::Value Sp3Api::SP3DynamicCast(const Napi::CallbackInfo& info)
{
  //auto object = PapyrusUtils::GetPapyrusValueFromJsValue(
   //   info[0], false, partOne->worldState);
  //  auto className = NapiHelper::ExtractString(info[1], "className");

  // TODO: implement
  // TODO: fix wip_sp3.js side of this function to properly wrap objects
  return info.Env().Undefined();
}
