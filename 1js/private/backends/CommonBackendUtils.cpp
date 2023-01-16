#include "CommonBackendUtils.h"
#include "JsValue.h"
#include <sstream>

std::string CommonBackendUtils::ConvertJsExceptionToString(void* exception)
{
    // TODO: refactor out "throw 1"
    try {
      auto stack = JsValue(exception).GetProperty("stack").ToString();
      if (stack == "undefined") {
        throw 1;
      }
      return stack;
    } catch (...) {
      std::stringstream ss;
      ss << JsValue(exception).ToString() << std::endl;
      ss << "<unable to get stack>";
      return ss.str();
    }
}