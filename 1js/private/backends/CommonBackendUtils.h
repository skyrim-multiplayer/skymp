#pragma once
#include <string>

#define JS_ENGINE_F(func) func, #func

class CommonBackendUtils {
public:
  static std::string ConvertJsExceptionToString(void* exception);
};
