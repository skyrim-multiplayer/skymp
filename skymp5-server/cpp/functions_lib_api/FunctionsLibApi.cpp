#include "FunctionsLibApi.h"
#include "FLActor.h"
#include "FLArmor.h"
#include "FLCell.h"
#include "FLForm.h"
#include "FLLocation.h"
#include "FLObjectReference.h"
#include "FLWeapon.h"

void RegisterFunctionsLibApi(std::shared_ptr<PartOne> partOne)
{
  RegisterFormApi(partOne);
  RegisterArmorApi(partOne);
  RegisterObjectReferenceApi(partOne);
  RegisterActorApi(partOne);
  RegisterCellApi(partOne);
  RegisterLocationApi(partOne);
  RegisterWeaponApi(partOne);
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

float FloatFromJsValue(const JsValue& v)
{
  if (v.GetType() == JsValue::Type::Number) {
    double formId = static_cast<double>(v);
    constexpr auto max =
      static_cast<double>(std::numeric_limits<uint32_t>::max());
    if (std::isfinite(formId) && formId >= 0 && formId < max) {
      return formId;
    }
  }
  return 0;
}
