#include "FLLocation.h"
#include "FormDesc.h"
#include "FunctionsLibApi.h"

void RegisterLocationApi(std::shared_ptr<PartOne> partOne)
{
  JsValue globalObject = JsValue::GlobalObject();

  // Form ctor
  globalObject.SetProperty(
    "Location", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return LocationCtor(partOne, args);
    }));

  JsValue location = globalObject.GetProperty("Location");
  location.SetProperty(
    "prototype", globalObject.GetProperty("Form").GetProperty("prototype"));

  JsValue locationPrototype = location.GetProperty("prototype");

  locationPrototype.SetProperty(
    "GetParent", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetParent(partOne, args);
    }));
}

JsValue LocationCtor(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[1]);
  if (formId == 0) {
    partOne->GetLogger().error("Error on check formId");
    return JsValue::Undefined();
  }

  args[0].SetProperty("_formId", args[1]);

  if (formId >= 0xff000000) {
    return JsValue::Undefined();
  }

  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);

  if (!lookupRes.rec || lookupRes.rec->GetType().ToString() != "LCTN") {
    partOne->GetLogger().error("Location not exists");
    return JsValue::Undefined();
  }

  return JsValue::Undefined();
}

JsValue GetParent(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  JsValue locationId = JsValue::Undefined();

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "PNAM") {
        locationId = JsValue::Int(*(uint32_t*)data);
      }
    },
    cache);

  return locationId;
}
