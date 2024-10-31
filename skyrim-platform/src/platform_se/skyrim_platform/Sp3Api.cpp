#include "Sp3Api.h"

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
  return info.Env().Undefined();
}

Napi::Value Sp3Api::SP3GetBaseClass(const Napi::CallbackInfo& info)
{
auto className = NapiHelper::ExtractString(info[0], "className");
  return info.Env().Undefined();
}

Napi::Value Sp3Api::SP3ListStaticFunctions(const Napi::CallbackInfo& info)
{
auto className = NapiHelper::ExtractString(info[0], "className");
  return info.Env().Undefined();
}

Napi::Value Sp3Api::SP3ListMethods(const Napi::CallbackInfo& info)
{
auto className = NapiHelper::ExtractString(info[0], "className");
  return info.Env().Undefined();
}

Napi::Value Sp3Api::SP3GetFunctionImplementation(const Napi::CallbackInfo& info)
{
auto className = NapiHelper::ExtractString(info[1], "className");
    auto functionName = NapiHelper::ExtractString(info[2], "functionName");
  return info.Env().Undefined();
}

Napi::Value Sp3Api::SP3DynamicCast(const Napi::CallbackInfo& info)
{
  //auto object = PapyrusUtils::GetPapyrusValueFromJsValue(
   //   info[0], false, partOne->worldState);
  //  auto className = NapiHelper::ExtractString(info[1], "className");
  return info.Env().Undefined();
}
