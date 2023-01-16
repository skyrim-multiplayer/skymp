#include "NodeApiBackendUtils.h"
#include "CommonBackend.h"
#include "JsExternalObjectBase.h"
#include "FunctionT.h"
#include "private/JsFunctionArgumentsImpl.h"
#include "JsValue.h" // Required for calling FunctionT
#include <sstream>

void NodeApiBackendUtils::Finalize(napi_env env, void *finalizeData, void *finalizeHint)
{
    auto finalizer = reinterpret_cast<CommonBackend::Finalize>(finalizeHint);
    auto data = reinterpret_cast<JsExternalObjectBase*>(finalizeData);
    finalizer(data);
}

std::string NodeApiBackendUtils::GetJsExceptionMessage(napi_env env, const char* operationName, napi_status errorCode)
{
    std::stringstream ss;
    napi_value exception;

    if (napi_get_and_clear_last_exception(env, &exception) == napi_status::napi_ok) {
        ss << ConvertJsExceptionToString(exception);
    } else {
        ss << "'" << operationName << "' returned error 0x" << std::hex << static_cast<int>(errorCode);
    }

    return ss.str();
}

napi_value NodeApiBackendUtils::NativeFunctionImpl(napi_env env, napi_callback_info info) {
    // get arguments count
    size_t argc = 0;
    auto errorCode = napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);
    if (errorCode != napi_status::napi_ok) {
        std::terminate();
    }

    // get arguments, thisArg and data
    std::vector<napi_value> arguments(argc + 1);
    void* data;
    errorCode = napi_get_cb_info(env, info, &argc, arguments.data() + 1, &arguments[0], &data);
    if (errorCode != napi_status::napi_ok) {
        std::terminate();
    }

    // construct JsFunctionArgumentsImpl
    auto argsStart = reinterpret_cast<void**>(arguments.data());
    JsFunctionArgumentsImpl jsArguments(argsStart, argc);

    // call function
    FunctionT* function = reinterpret_cast<FunctionT*>(data);
    try {
        JsValue result = (*function)(jsArguments);
        return static_cast<napi_value>(result.value);
    }
    catch (std::exception &e) {
        napi_throw_error(env, nullptr, e.what());
        return nullptr;
    }
}
