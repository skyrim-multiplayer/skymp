#include "AnyBackend.h"
#include "NodeApiBackend.h"
#include "NodeApiBackendUtils.h"
#include "JsEngine.h"
#include <cstring>
#include <variant>

namespace {
    struct NullT {};
    struct UndefinedT {};

    using Variant = std::variant<napi_ref, std::string, int, double, bool, NullT, UndefinedT>;
    
    struct ValueImpl {
        Variant value = UndefinedT{};
        int64_t refCount = 0;

        // explicit ValueImpl(napi_value value) {
        //     napi_valuetype type;
        //     NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_typeof), g_env, value, &type);
        //     switch(type) {
        //         case napi_undefined:
        //             this->value = Undefined{};
        //             return;
        //         case napi_null:
        //             this->value = Null{};
        //             return;
        //         case napi_boolean: {
        //             bool b;
        //             NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_bool), g_env, value, &b);
        //             this->value = b;
        //             return;
        //         }
        //         case napi_number: {
        //             double d;
        //             NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_double), g_env, value, &d);
        //             if (d == static_cast<int>(d)) {
        //                 this->value = static_cast<int>(d);
        //             }
        //             else {
        //                 this->value = d;
        //             }
        //             return;
        //         }
        //         case napi_string: {
        //             size_t len;
        //             NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_string_utf8), g_env, value, nullptr, 0, &len);
        //             std::string str(len, '\0');
        //             NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_string_utf8), g_env, value, str.data(), len + 1, &len);
        //             this->value = std::move(str);
        //             return;
        //         }
        //         // TODO: verify that napi_create_reference works for napi_symbol
        //         case napi_symbol:
        //         case napi_object:
        //         case napi_function:
        //         case napi_external: {
        //             napi_ref ref;
        //             NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_reference), g_env, value, 1, &ref);
        //             this->value = ref;
        //             return;
        //         }
        //         default:
        //             return;
        //         }
        //     }
    };
}

AnyBackend_DefineCreateFunction(MakeNodeApiBackend, NodeApiBackend);

JsEngine JsEngine::CreateNodeApi(void* env) {
    AnyBackend::GetInstanceForCurrentThread() = AnyBackend::MakeNodeApiBackend();
    return JsEngine(env);
}

thread_local napi_env g_env;

void NodeApiBackend::Create(void* env) {
    g_env = reinterpret_cast<napi_env>(env);
}

void NodeApiBackend::Destroy() {
    g_env = nullptr;
}

void NodeApiBackend::ResetContext(Viet::TaskQueue &) {
    // Nothing to do
}

void* NodeApiBackend::RunScript(const char *src, const char *) {
    napi_value script = static_cast<napi_value>(String(src));

    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_run_script), g_env, script, &result);

    return result;
}

size_t NodeApiBackend::GetMemoryUsage() {
    return 0;
}

void* NodeApiBackend::Undefined() {
    auto result = new ValueImpl();
    result->value = UndefinedT{};
    return result;
}

void* NodeApiBackend::Null() {
    auto result = new ValueImpl();
    result->value = NullT{};
    return result;
}

void* NodeApiBackend::Object() {
    napi_value object;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_object), g_env, &object);

    auto result = new ValueImpl();
    result->value = napi_ref{};
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_reference), g_env, object, 1, &std::get<napi_ref>(result->value));
    return result;
}

void* NodeApiBackend::ExternalObject(JsExternalObjectBase *data, std::optional<Finalize> finalize) {
    napi_value result;
    if (finalize) {
        NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_external), g_env, data, NodeApiBackendUtils::Finalize, reinterpret_cast<void*>(*finalize), &result);
    }
    else {
        NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_external), g_env, data, nullptr, nullptr, &result);
    }
    return result;
}

void* NodeApiBackend::Array(uint32_t n) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_array_with_length), g_env, n, &result);
    return result;
}

void* NodeApiBackend::GlobalObject() {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_global), g_env, &result);
    return result;
}

void* NodeApiBackend::Bool(bool arg) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_boolean), g_env, arg, &result);
    return result;
}

void* NodeApiBackend::String(const std::string &arg) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_string_utf8), g_env, arg.c_str(), arg.size(), &result);
    return result;
}

void* NodeApiBackend::Int(int arg) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_int32), g_env, arg, &result);
    return result;
}

void* NodeApiBackend::Double(double arg) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_double), g_env, arg, &result);
    return result;
}

// TODO: fix memory leak (new FunctionT(arg)), node-addon-api may have a reference on how to do this
void* NodeApiBackend::Function(const FunctionT &arg) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_function), g_env, nullptr, 0, NodeApiBackendUtils::NativeFunctionImpl, new FunctionT(arg), &result);
    return result;
}

// TODO: fix memory leak (new FunctionT(arg)), node-addon-api may have a reference on how to do this
void* NodeApiBackend::NamedFunction(const char *name, const FunctionT &arg) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_function), g_env, name, strlen(name), NodeApiBackendUtils::NativeFunctionImpl, new FunctionT(arg), &result);
    return result;
}

void* NodeApiBackend::Uint8Array(uint32_t length) {
    napi_value arrayBuffer;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_arraybuffer), g_env, length, nullptr, &arrayBuffer);

    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_typedarray), g_env, napi_uint8_array, length, arrayBuffer, 0, &result);
    return result;
}

void* NodeApiBackend::ArrayBuffer(uint32_t length) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_arraybuffer), g_env, length, nullptr, &result);
    return result;
}

void* NodeApiBackend::GetTypedArrayData(void *value_) {
    napi_value value = reinterpret_cast<napi_value>(value_);
    napi_typedarray_type type;
    size_t length;
    void *data;
    napi_value arrayBuffer;
    size_t byteOffset;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_typedarray_info), g_env, value, &type, &length, &data, &arrayBuffer, &byteOffset);
    return data;
}

uint32_t NodeApiBackend::GetTypedArrayBufferLength(void *value_) {
    napi_value value = reinterpret_cast<napi_value>(value_);
    napi_typedarray_type type;
    size_t length;
    void *data;
    napi_value arrayBuffer;
    size_t byteOffset;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_typedarray_info), g_env, value, &type, &length, &data, &arrayBuffer, &byteOffset);
    switch(type) {
        case napi_int8_array:
        case napi_uint8_array:
        case napi_uint8_clamped_array:
            return length;
        case napi_int16_array:
        case napi_uint16_array:
            return length * 2;
        case napi_int32_array:
        case napi_uint32_array:
        case napi_float32_array:
            return length * 4;
        case napi_float64_array:
            return length * 8;
        default:
            return 0;
    }
}   

void* NodeApiBackend::GetArrayBufferData(void *value_) {
    auto value = static_cast<napi_value>(value_);
    size_t length;
    void *data;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_arraybuffer_info), g_env, value, &data, &length);
    return data;
}

uint32_t NodeApiBackend::GetArrayBufferLength(void *value_) {
    auto value = static_cast<napi_value>(value_);
    size_t length;
    void *data;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_arraybuffer_info), g_env, value, &data, &length);
    return length;
}

void* NodeApiBackend::ConvertValueToString(void *value_) {
    auto value = static_cast<napi_value>(value_);
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_coerce_to_string), g_env, value, &result);
    return result;
}

std::string NodeApiBackend::GetString(void *value_) {
    auto value = static_cast<napi_value>(value_);
    size_t length;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_string_utf8), g_env, value, nullptr, 0, &length);
    std::string result(length, '\0');
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_string_utf8), g_env, value, &result[0], length + 1, &length);
    return result;
}

bool NodeApiBackend::GetBool(void *value_) {
    auto value = static_cast<napi_value>(value_);
    bool result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_bool), g_env, value, &result);
    return result;
}

int NodeApiBackend::GetInt(void *value_) {
    napi_value value = static_cast<napi_value>(value_);
    int32_t result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_int32), g_env, value, &result);
    return result;
}

double NodeApiBackend::GetDouble(void *value_) {
    napi_value value = static_cast<napi_value>(value_);
    double result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_double), g_env, value, &result);
    return result;
}

JsType NodeApiBackend::GetType(void *value_) {
    napi_value value = static_cast<napi_value>(value_);

    // TODO: replace napi_typeof with napi_is_* functions for numbers, strings, etc.
    // Check for popular types first

    bool isError;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_is_error), g_env, value, &isError);
    if (isError) {
        return JsType::Error;
    }

    bool isArray;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_is_array), g_env, value, &isArray);
    if (isArray) {
        return JsType::Array;
    }

    bool isArrayBuffer;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_is_arraybuffer), g_env, value, &isArrayBuffer);
    if (isArrayBuffer) {
        return JsType::ArrayBuffer;
    }

    bool isTypedArray;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_is_typedarray), g_env, value, &isTypedArray);
    if (isTypedArray) {
        return JsType::TypedArray;
    }

    bool isDataView;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_is_dataview), g_env, value, &isDataView);
    if (isDataView) {
        return JsType::DataView;
    }

    napi_valuetype type;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_typeof), g_env, value, &type);
    switch(type) {
        case napi_undefined:
            return JsType::Undefined;
        case napi_null:
            return JsType::Null;
        case napi_boolean:
            return JsType::Boolean;
        case napi_number:
            return JsType::Number;
        case napi_string:
            return JsType::String;
        case napi_symbol:
            return JsType::Symbol;
        case napi_object:
            return JsType::Object;
        case napi_function:
            return JsType::Function;
        case napi_external:
            // Originally we treeted external as object in JsEngine
            return JsType::Object;
        default:
            return JsType::Undefined;
    }
}

JsExternalObjectBase* NodeApiBackend::GetExternalData(void *value_) {
    napi_value value = static_cast<napi_value>(value_);

    napi_valuetype type;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_typeof), g_env, value, &type);
    if (type != napi_external) {
        return nullptr;
    }

    JsExternalObjectBase *result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_external), g_env, value, reinterpret_cast<void**>(&result));
    return result;
}

void NodeApiBackend::SetProperty(void *value_, void* key_, void *newValue_) {
    napi_value value = static_cast<napi_value>(value_);
    napi_value key = static_cast<napi_value>(key_);
    napi_value newValue = static_cast<napi_value>(newValue_);

    napi_valuetype type;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_typeof), g_env, key, &type);
    switch (type) {
        case napi_number: {
            int32_t index;
            NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_int32), g_env, key, &index);
            NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_set_element), g_env, value, index, newValue);
        } break;
        case napi_string: {
            auto str = GetString(key);
            NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_set_named_property), g_env, value, str.c_str(), newValue);
        } break;
        default:
            throw std::runtime_error("SetProperty: Bad key type (" +
                                    std::to_string(int(type)) + ")");
    }
}

// TODO: fix memory leak. It uses Function method which creates new std::function every time
void NodeApiBackend::DefineProperty(void *value, void* key, const FunctionT &getter, const FunctionT &setter) {
    // TODO: rewrite without eval
    auto src = "(value, key, getter, setter) => Object.defineProperty(value, key, { getter, setter });";
    napi_value script = static_cast<napi_value>(String(src));
    napi_value definePropertyFunction;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_run_script), g_env, script, &definePropertyFunction);

    napi_value args[4] = {
        static_cast<napi_value>(value),
        static_cast<napi_value>(key),
        static_cast<napi_value>(Function(getter)),
        static_cast<napi_value>(Function(setter))
    };

    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_call_function), g_env, nullptr, definePropertyFunction, 4, args, &result);
}

void* NodeApiBackend::GetProperty(void *value_, void *key_) {
    auto value = static_cast<napi_value>(value_);
    auto key = static_cast<napi_value>(key_);

    napi_valuetype type;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_typeof), g_env, key, &type);
    switch (type) {
        case napi_number: {
            int32_t index;
            NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_int32), g_env, key, &index);
            napi_value res;
            NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_element), g_env, value, index, &res);
            return res;
        }
        case napi_string: {
            auto str = GetString(key);
            napi_value res;
            NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_named_property), g_env, value, str.c_str(), &res);
            return res;
        }
        case napi_symbol: {
            napi_value res;
            NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_property), g_env, value, key, &res);
            return res;
        }
        default:
            throw std::runtime_error("GetProperty: Bad key type (" +
                                    std::to_string(static_cast<int>(type)) + ")");
    }
}

void* NodeApiBackend::Call(void *value, void** arguments_, uint32_t argumentCount, bool isConstructor) {
    napi_value *arguments = reinterpret_cast<napi_value*>(arguments_);
    napi_value functionToCall = static_cast<napi_value>(value);

    napi_value thisArg;
    const napi_value *argumentsNoThis;
    size_t argumentsNoThisCount;
    if (argumentCount == 0) {
        thisArg = static_cast<napi_value>(Undefined());
        argumentsNoThis = nullptr;
        argumentsNoThisCount = 0;
    }
    else if (argumentCount == 1) {
        thisArg = arguments[0];
        argumentsNoThis = nullptr;
        argumentsNoThisCount = 0;
    }
    else {
        thisArg = arguments[0];
        argumentsNoThis = arguments + 1;
        argumentsNoThisCount = argumentCount - 1;
    }

    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_call_function), g_env, thisArg, functionToCall, argumentsNoThisCount, argumentsNoThis, &result);
    return result;
}

void NodeApiBackend::AddRef(void *value) {
    napi_create_reference;
    // NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_reference_ref), g_env, static_cast<napi_ref>(value), nullptr);
}

void NodeApiBackend::Release(void *value) {
    // NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_reference_unref), g_env, static_cast<napi_ref>(value), nullptr);
}
