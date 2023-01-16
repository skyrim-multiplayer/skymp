#include "NodeApiBackendUtils.h"
#include "CommonBackend.h"
#include "JsExternalObjectBase.h"
#include <sstream>
#include <napi.h>

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