#include "FLArmor.h"
#include "FormDesc.h"
#include "FunctionsLibApi.h"

void RegisterArmorApi(std::shared_ptr<PartOne> partOne)
{
  JsValue globalObject = JsValue::GlobalObject();

  // Form ctor
  globalObject.SetProperty(
    "Armor", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return ArmorCtor(partOne, args);
    }));

  JsValue armor = globalObject.GetProperty("Armor");
  armor.SetProperty("prototype",
                    globalObject.GetProperty("Form").GetProperty("prototype"));

  JsValue armorPrototype = armor.GetProperty("prototype");

  armorPrototype.SetProperty(
    "GetArmorRating",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetArmorRating(partOne, args);
    }));

  armorPrototype.SetProperty(
    "GetSlot", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetSlot(partOne, args);
    }));
}

JsValue ArmorCtor(std::shared_ptr<PartOne> partOne,
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

  if (!lookupRes.rec || lookupRes.rec->GetType().ToString() == "") {
    partOne->GetLogger().error("Form not exists");
    return JsValue::Undefined();
  }

  return JsValue::Undefined();
}

JsValue GetArmorRating(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  float rating = 0;

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "DNAM") {
        rating = *(uint32_t*)data;
      }
    },
    cache);

  return JsValue::Double(rating);
}

JsValue GetSlot(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  uint32_t slot = 0;

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "BOD2") {
        slot = *(uint32_t*)data;
      }
    },
    cache);

  return JsValue::Int(slot);
}
