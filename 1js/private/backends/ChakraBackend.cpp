#include "ChakraBackend.h"
#include "ChakraBackendUtils.h"
#include <cstring>
#include <ChakraCore.h>

thread_local unsigned g_currentSourceContext = 0;
thread_local JsRuntimeHandle g_runtime = nullptr;
thread_local JsContextRef g_context = nullptr;

void ChakraBackend::Create() {
    JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &g_runtime);
}

void ChakraBackend::Destroy() {
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    JsDisposeRuntime(g_runtime);
}

void ChakraBackend::ResetContext(Viet::TaskQueue &taskQueue) {
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateContext), g_runtime,
                      &g_context);
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsSetCurrentContext), g_context);

    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsSetPromiseContinuationCallback),
                      ChakraBackendUtils::OnPromiseContinuation, &taskQueue);

    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsSetHostPromiseRejectionTracker),
                      ChakraBackendUtils::OnPromiseRejection, &taskQueue);
}

void *ChakraBackend::RunScript(const char *src, const char *fileName) {
    JsValueRef scriptSource;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateExternalArrayBuffer),
                      reinterpret_cast<void*>(const_cast<char *>(src)),
                      static_cast<unsigned int>(strlen(src)),
                      nullptr, nullptr, &scriptSource);

    JsValueRef fname;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateString), fileName,
                      strlen(fileName), &fname);

    JsValueRef result = nullptr;
    auto jsRunRes = JsRun(scriptSource, g_currentSourceContext++, fname,
                          JsParseScriptAttributeNone, &result);
    if (jsRunRes != JsNoError) {
      JsValueRef exception = nullptr;
      if (JsGetAndClearException(&exception) == JsNoError) {
        std::string str = ChakraBackendUtils::ConvertJsExceptionToString(exception);
        throw std::runtime_error(str);
      } else {
        throw std::runtime_error("JsRun failed");
      }
    }

    return result;
}

size_t ChakraBackend::GetMemoryUsage() {
    size_t res;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetRuntimeMemoryUsage), g_runtime,
                      &res);
    return res;
}

void *ChakraBackend::Undefined() {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetUndefinedValue), &v);
    return v;
}

void *ChakraBackend::Null() {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetNullValue), &v);
    return v;
}

void *ChakraBackend::Object() {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateObject), &v);
    return v;
}

void *ChakraBackend::ExternalObject(JsExternalObjectBase *data) {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateExternalObject), data, nullptr, &v);
    return v;
}

void *ChakraBackend::Array(uint32_t n) {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateArray), n, &v);
    return v;
}

void *ChakraBackend::GlobalObject() {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetGlobalObject), &v);
    return v;
}

void *ChakraBackend::Bool(bool arg) {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsBoolToBoolean), arg, &v);
    return v;
}

void *ChakraBackend::String(const std::string &arg) {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateString), arg.data(), arg.size(), &v);
    return v;
}

void *ChakraBackend::Int(int arg) {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsIntToNumber), arg, &v);
    return v;
}

void *ChakraBackend::Double(double arg) {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsDoubleToNumber), arg, &v);
    return v;
}

// TODO: fix memory leak (new FunctionT(arg))
void *ChakraBackend::Function(const FunctionT &arg) {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateFunction), ChakraBackendUtils::NativeFunctionImpl,
             new FunctionT(arg), &v);
    return v;
}

// TODO: fix memory leak (new FunctionT(arg))
void *ChakraBackend::NamedFunction(const char *name, const FunctionT &arg) {
    JsValueRef v;
    auto jsName = String(name);
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateNamedFunction), jsName,
             ChakraBackendUtils::NativeFunctionImpl, new FunctionT(arg), &v);
    return v;
}

void *ChakraBackend::Uint8Array(uint32_t length) {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateTypedArray),
             JsTypedArrayType::JsArrayTypeUint8, JS_INVALID_REFERENCE, 0,
             length, &v);
    return v;
}

void *ChakraBackend::ArrayBuffer(uint32_t length) {
    JsValueRef v;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreateArrayBuffer), length, &v);
    return v;
}

void *ChakraBackend::GetTypedArrayData(void *value) {
    ChakraBytePtr chakraBytePtr = nullptr;
    unsigned int bufferLength = 0;
    JsTypedArrayType typedArrayType = JsTypedArrayType::JsArrayTypeFloat32;
    int elementSize = 0;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetTypedArrayStorage), value, &chakraBytePtr,
             &bufferLength, &typedArrayType, &elementSize);
    return chakraBytePtr;
}

uint32_t ChakraBackend::GetTypedArrayBufferLength(void *value) {
    ChakraBytePtr chakraBytePtr = nullptr;
    unsigned int bufferLength = 0;
    JsTypedArrayType typedArrayType = JsTypedArrayType::JsArrayTypeFloat32;
    int elementSize = 0;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetTypedArrayStorage), value, &chakraBytePtr,
             &bufferLength, &typedArrayType, &elementSize);
    return bufferLength;
}

void *ChakraBackend::GetArrayBufferData(void *value) {
    ChakraBytePtr chakraBytePtr = nullptr;
    unsigned int bufferLength = 0;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetArrayBufferStorage), value, &chakraBytePtr,
             &bufferLength);
    return chakraBytePtr;
}

uint32_t ChakraBackend::GetArrayBufferLength(void* value)
  {
    ChakraBytePtr chakraBytePtr = nullptr;
    unsigned int bufferLength = 0;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetArrayBufferStorage), value, &chakraBytePtr,
             &bufferLength);
    return bufferLength;
  }

void* ChakraBackend::ConvertValueToString(void *value) {
  JsValueRef res;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsConvertValueToString), value, &res);
    return res;
}

std::string ChakraBackend::GetString(void* value) {
  size_t outLength;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCopyString), value, nullptr, 0, &outLength);

    std::string res;
    res.resize(outLength);
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCopyString), value, res.data(), outLength,
             &outLength);
    return res;
}

bool ChakraBackend::GetBool(void *value) {
    bool res;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsBooleanToBool), value, &res);
    return res;
}

int ChakraBackend::GetInt(void *value) {
    int res;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsNumberToInt), value, &res);
    return res;
}

double ChakraBackend::GetDouble(void *value) {
    double res;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsNumberToDouble), value, &res);
    return res;
}

JsType ChakraBackend::GetType(void *value) {
    JsValueType type;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetValueType), value, &type);
    switch (type) {
        case JsUndefined:
            return JsType::Undefined;
        case JsNull:
            return JsType::Null;
        case JsNumber:
            return JsType::Number;
        case JsString:
            return JsType::String;
        case JsBoolean:
            return JsType::Boolean;
        case JsObject:
            return JsType::Object;
        case JsFunction:
            return JsType::Function;
        case JsError:
            return JsType::Error;
        case JsArray:
            return JsType::Array;
        case JsSymbol:
            return JsType::Symbol;
        case JsArrayBuffer:
            return JsType::ArrayBuffer;
        case JsTypedArray:
            return JsType::TypedArray;
        case JsDataView:
            return JsType::DataView;
        default:
            return JsType::Undefined;
    }
}

JsExternalObjectBase *ChakraBackend::GetExternalData(void *value) {
    void* externalData;
    bool hasExternslData;

    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsHasExternalData), value, &hasExternslData);
    if (!hasExternslData) {
      return nullptr;
    }

    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetExternalData), value, &externalData);
    return reinterpret_cast<JsExternalObjectBase*>(externalData);
}

void ChakraBackend::SetProperty(void *value, void* key, void *newValue) {
  JsType type = GetType(key);
  switch (type) {
    case JsType::Number: {
      ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsSetIndexedProperty), value, key, newValue);
    } break;
    case JsType::String: {
      auto str = GetString(key);
      JsPropertyIdRef propId;
      ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreatePropertyId), str.data(), str.size(),
                  &propId);
      ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsSetProperty), value, propId, newValue,
                  true);
    } break;
    default:
      throw std::runtime_error("SetProperty: Bad key type (" +
                               std::to_string(int(type)) + ")");
  }
}

void ChakraBackend::DefineProperty(void *value, void* key, void* descriptor) {
  bool result;
  ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsObjectDefineProperty), value, key, descriptor, &result);
}

void *ChakraBackend::GetProperty(void *value, void *key) {
    JsType type = GetType(key);
    switch (type) {
      case JsType::Number: {
        JsValueRef res;
        ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetIndexedProperty), value, key, &res);
        return res;
      }
      case JsType::String: {
        JsPropertyIdRef propId;
        JsValueRef res;

        // Hot path. Platform-specific functions on Windows are faster than the
        // cross-platform equivalents
#ifndef WIN32
        auto str = GetString(key);
        ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsCreatePropertyId), str.data(), str.size(),
                 &propId);
#else
        const wchar_t* stringPtr;
        size_t stringSize;
        ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsStringToPointer), key, &stringPtr,
                 &stringSize);
        ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetPropertyIdFromName), stringPtr, &propId);
#endif
        ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetProperty), value, propId, &res);
        return res;
      }
      case JsType::Symbol: {
        JsValueRef res;
        JsPropertyIdRef propId;
        ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetPropertyIdFromSymbol), key, &propId);
        ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetProperty), value, propId, &res);
        return res;
      }
      default:
        throw std::runtime_error("GetProperty: Bad key type (" +
                                 std::to_string(static_cast<int>(type)) + ")");
    }
}

void* ChakraBackend::Call(void *value, void** arguments, uint32_t argumentCount, bool isConstructor) {
    JsValueRef res;

    JsValueRef undefined;
    ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsGetUndefinedValue), &undefined);

    JsValueRef* args = nullptr;

    if (argumentCount > 0) {
      args = arguments;
    } else {
      args = &undefined;
      ++argumentCount;
    }

    ChakraBackendUtils::SafeCall(isConstructor ? JsConstructObject : JsCallFunction,
             "JsCallFunction/JsConstructObject", value, args, argumentCount, &res);
    return res;
}

void ChakraBackend::AddRef(void *value) {
  ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsAddRef), value, nullptr);
}

void ChakraBackend::Release(void *value) {
  ChakraBackendUtils::SafeCall(JS_ENGINE_F(JsRelease), value, nullptr);
}
