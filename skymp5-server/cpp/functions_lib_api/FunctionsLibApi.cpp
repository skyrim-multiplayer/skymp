#include "FunctionsLibApi.h"
#include "FLForm.h"

void RegisterFunctionsLibApi(std::shared_ptr<PartOne> partOne)
{
  RegisterFormApi(partOne);
}

uint32_t Uint32FromJsValue(const JsValue& v)
{
  if (v.GetType() == JsValue::Type::Number) {
    double formId = static_cast<double>(v);
    constexpr auto max =
      static_cast<double>(std::numeric_limits<uint32_t>::max());
    if (std::isfinite(formId) && formId >= 0 && formId < max) {
      return static_cast<uint32_t>(floor(formId));
    }
  }
  return 0;
}
