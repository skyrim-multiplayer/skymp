#include "FLWeapon.h"
#include "FormDesc.h"
#include "FunctionsLibApi.h"

void RegisterWeaponApi(std::shared_ptr<PartOne> partOne)
{
  JsValue globalObject = JsValue::GlobalObject();

  // Form ctor
  globalObject.SetProperty(
    "Weapon", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return WeaponCtor(partOne, args);
    }));

  JsValue weapon = globalObject.GetProperty("Weapon");
  weapon.SetProperty(
    "prototype", globalObject.GetProperty("Form").GetProperty("prototype"));

  JsValue weaponPrototype = weapon.GetProperty("prototype");

  weaponPrototype.SetProperty(
    "GetBaseDamage",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetBaseDamage(partOne, args);
    }));
}

JsValue WeaponCtor(std::shared_ptr<PartOne> partOne,
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

  if (!lookupRes.rec || lookupRes.rec->GetType().ToString() != "WEAP") {
    partOne->GetLogger().error("Weapon not exists");
    return JsValue::Undefined();
  }

  return JsValue::Undefined();
}

JsValue GetBaseDamage(std::shared_ptr<PartOne> partOne,
                      const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  JsValue damage = JsValue::Undefined();

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "DATA") {
        damage = JsValue::Int(*(uint32_t*)&data[8]);
      }
    },
    cache);

  return damage;
}
