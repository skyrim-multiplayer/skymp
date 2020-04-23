#pragma once
#include "JsEngine.h"
#include "TaskQueue.h"

namespace VmApi {
JsValue GetTypesCount(const JsFunctionArguments& args);
JsValue GetTypeName(const JsFunctionArguments& args);
JsValue GetTypeStatesCount(const JsFunctionArguments& args);
JsValue GetTypeFunctionsCount(const JsFunctionArguments& args);
JsValue GetTypeMethodsCount(const JsFunctionArguments& args);
JsValue GetMethodProperty(const JsFunctionArguments& args);
JsValue GetFunctionProperty(const JsFunctionArguments& args);
JsValue Call(const JsFunctionArguments& args);

extern TaskQueue* taskQueue;

inline void Register(JsValue& exports, TaskQueue* taskQueue_)
{
  taskQueue = taskQueue_;

  exports.SetProperty("getTypesCount", JsValue::Function(GetTypesCount));
  exports.SetProperty("getTypeName", JsValue::Function(GetTypeName));
  exports.SetProperty("getTypeStatesCount",
                      JsValue::Function(GetTypeStatesCount));
  exports.SetProperty("getTypeFunctionsCount",
                      JsValue::Function(GetTypeFunctionsCount));
  exports.SetProperty("getTypeMethodsCount",
                      JsValue::Function(GetTypeMethodsCount));
  exports.SetProperty("getMethodProperty",
                      JsValue::Function(GetMethodProperty));
  exports.SetProperty("getFunctionProperty",
                      JsValue::Function(GetFunctionProperty));
  exports.SetProperty("call", JsValue::Function(Call));
}
}