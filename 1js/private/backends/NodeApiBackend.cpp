#include "NodeApiBackend.h"
#include "NodeApiBackendUtils.h"
#include <napi.h>
#include <cstring>

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
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_undefined), g_env, &result);
    return result;
}

void* NodeApiBackend::Null() {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_null), g_env, &result);
    return result;
}

void* NodeApiBackend::Object() {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_object), g_env, &result);
    return result;
}

void* NodeApiBackend::ExternalObject(JsExternalObjectBase *data, std::optional<Finalize> finalize) {
    napi_value result;
    if (finalize) {
        NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_external), g_env, data, NodeApiBackendUtils::Finalize, *finalize, &result);
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
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_arraybuffer), g_env, length, &arrayBuffer);

    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_typedarray), g_env, napi_uint8_array, length, arrayBuffer, 0, &result);
    return result;
}

void* NodeApiBackend::ArrayBuffer(uint32_t length) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_create_arraybuffer), g_env, length, &result);
    return result;
}

void* NodeApiBackend::GetTypedArrayData(void *value) {
    napi_typedarray_type type;
    size_t length;
    void *data;
    napi_value arrayBuffer;
    size_t byteOffset;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_typedarray_info), g_env, value, &type, &length, &data, &arrayBuffer, &byteOffset);
    return data;
}

uint32_t NodeApiBackend::GetTypedArrayBufferLength(void *value) {
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

void* NodeApiBackend::GetArrayBufferData(void *value) {
    size_t length;
    void *data;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_arraybuffer_info), g_env, value, &data, &length);
    return data;
}

uint32_t NodeApiBackend::GetArrayBufferLength(void *value) {
    size_t length;
    void *data;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_arraybuffer_info), g_env, value, &data, &length);
    return length;
}

void* NodeApiBackend::ConvertValueToString(void *value) {
    napi_value result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_coerce_to_string), g_env, value, &result);
    return result;
}

std::string NodeApiBackend::GetString(void *value) {
    size_t length;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_string_utf8), g_env, value, nullptr, 0, &length);
    std::string result(length, '\0');
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_string_utf8), g_env, value, &result[0], length + 1, &length);
    return result;
}

bool NodeApiBackend::GetBool(void *value) {
    bool result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_bool), g_env, value, &result);
    return result;
}

int NodeApiBackend::GetInt(void *value) {
    int32_t result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_int32), g_env, value, &result);
    return result;
}

double NodeApiBackend::GetDouble(void *value) {
    double result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_double), g_env, value, &result);
    return result;
}

JsType NodeApiBackend::GetType(void *value) {
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

JsExternalObjectBase* NodeApiBackend::GetExternalData(void *value) {
    napi_valuetype type;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_typeof), g_env, value, &type);
    if (type != napi_external) {
        return nullptr;
    }

    JsExternalObjectBase *result;
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_value_external), g_env, value, reinterpret_cast<void**>(&result));
    return result;
}

void NodeApiBackend::SetProperty(void *value, void* key, void *newValue) {
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

void NodeApiBackend::DefineProperty(void *value, void* key, void* descriptor) {
    // // get getter and setter from descriptor
    // napi_value getter;
    // NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_named_property), g_env, descriptor, "get", &getter);
    // napi_value setter;
    // NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_get_named_property), g_env, descriptor, "set", &setter);

    // napi_property_descriptor desc = {
    // nullptr,
    // static_cast<napi_value>(key),
    // nullptr, //method
    // getter,
    // setter,
    // nullptr,
    // napi_writable | napi_enumerable | napi_configurable,
    // nullptr
    // };

    // NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_define_properties), g_env, value, 1, &desc);
}

void* NodeApiBackend::GetProperty(void *value, void *key) {
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

void* NodeApiBackend::Call(void *value, void** arguments, uint32_t argumentCount, bool isConstructor) {

}

void NodeApiBackend::AddRef(void *value) {
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_reference_ref), g_env, static_cast<napi_ref>(value), nullptr);
}

void NodeApiBackend::Release(void *value) {
    NodeApiBackendUtils::SafeCall(JS_ENGINE_F(napi_reference_unref), g_env, static_cast<napi_ref>(value), nullptr);
}