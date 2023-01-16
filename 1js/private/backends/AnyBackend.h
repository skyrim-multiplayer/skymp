#pragma once
#include "CommonBackend.h"
#include "FunctionT.h"
#include "JsExternalObjectBase.h"
#include "JsType.h"
#include "TaskQueue.h"
#include <string>
#include <optional>

#define BACKEND AnyBackend::GetInstanceForCurrentThread().
// #define BACKEND ChakraBackend::
// #define BACKEND NodeApiBackend::

class AnyBackend {
public:
    static AnyBackend &GetInstanceForCurrentThread();

    static AnyBackend MakeChakraBackend();
    static AnyBackend MakeNodeApiBackend();

    using Finalize = CommonBackend::Finalize;

    void(*Create)(void*);
    void(*Destroy)();
    void(*ResetContext)(Viet::TaskQueue &taskQueue);
    void *(*RunScript)(const char *src, const char *fileName);
    size_t(*GetMemoryUsage)();
    void *(*Undefined)();
    void *(*Null)();
    void *(*Object)();
    void *(*ExternalObject)(JsExternalObjectBase *data, std::optional<Finalize> finalize);
    void *(*Array)(uint32_t n);
    void *(*GlobalObject)();
    void *(*Bool)(bool arg);
    void *(*String)(const std::string &arg);
    void *(*Int)(int arg);
    void *(*Double)(double arg);
    void *(*Function)(const FunctionT &arg);
    void *(*NamedFunction)(const char *name, const FunctionT &arg);
    void *(*Uint8Array)(uint32_t length);
    void *(*ArrayBuffer)(uint32_t length);
    void *(*GetTypedArrayData)(void *value);
    uint32_t(*GetTypedArrayBufferLength)(void *value);
    void *(*GetArrayBufferData)(void *value);
    uint32_t(*GetArrayBufferLength)(void *value);
    void *(*ConvertValueToString)(void *value);
    std::string(*GetString)(void *value);
    bool(*GetBool)(void *value);
    int(*GetInt)(void *value);
    double(*GetDouble)(void *value);
    JsType(*GetType)(void *value);
    JsExternalObjectBase *(*GetExternalData)(void *value);
    void(*SetProperty)(void *value, void* key, void *newValue);
    void(*DefineProperty)(void *value, void* key, const FunctionT &getter, const FunctionT &setter);
    void *(*GetProperty)(void *value, void *key);
    void *(*Call)(void *value, void** arguments, uint32_t argumentCount, bool isConstructor);
    void(*AddRef)(void *value);
    void(*Release)(void *value);
};

#define AnyBackend_DefineCreateFunction(FUNCTION_NAME, BACKEND) \
AnyBackend AnyBackend::FUNCTION_NAME() {\
    AnyBackend backend;\
    backend.Create = BACKEND::Create;\
    backend.Destroy = BACKEND::Destroy;\
    backend.ResetContext = BACKEND::ResetContext;\
    backend.RunScript = BACKEND::RunScript;\
    backend.GetMemoryUsage = BACKEND::GetMemoryUsage;\
    backend.Undefined = BACKEND::Undefined;\
    backend.Null = BACKEND::Null;\
    backend.Object = BACKEND::Object;\
    backend.ExternalObject = BACKEND::ExternalObject;\
    backend.Array = BACKEND::Array;\
    backend.GlobalObject = BACKEND::GlobalObject;\
    backend.Bool = BACKEND::Bool;\
    backend.String = BACKEND::String;\
    backend.Int = BACKEND::Int;\
    backend.Double = BACKEND::Double;\
    backend.Function = BACKEND::Function;\
    backend.NamedFunction = BACKEND::NamedFunction;\
    backend.Uint8Array = BACKEND::Uint8Array;\
    backend.ArrayBuffer = BACKEND::ArrayBuffer;\
    backend.GetTypedArrayData = BACKEND::GetTypedArrayData;\
    backend.GetTypedArrayBufferLength = BACKEND::GetTypedArrayBufferLength;\
    backend.GetArrayBufferData = BACKEND::GetArrayBufferData;\
    backend.GetArrayBufferLength = BACKEND::GetArrayBufferLength;\
    backend.ConvertValueToString = BACKEND::ConvertValueToString;\
    backend.GetString = BACKEND::GetString;\
    backend.GetBool = BACKEND::GetBool;\
    backend.GetInt = BACKEND::GetInt;\
    backend.GetDouble = BACKEND::GetDouble;\
    backend.GetType = BACKEND::GetType;\
    backend.GetExternalData = BACKEND::GetExternalData;\
    backend.SetProperty = BACKEND::SetProperty;\
    backend.DefineProperty = BACKEND::DefineProperty;\
    backend.GetProperty = BACKEND::GetProperty;\
    backend.Call = BACKEND::Call;\
    backend.AddRef = BACKEND::AddRef;\
    backend.Release = BACKEND::Release;\
    return backend;\
}