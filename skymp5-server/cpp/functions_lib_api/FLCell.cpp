#include "FLCell.h"
#include "FormDesc.h"
#include "FunctionsLibApi.h"

void RegisterCellApi(std::shared_ptr<PartOne> partOne)
{
  JsValue globalObject = JsValue::GlobalObject();

  // Form ctor
  globalObject.SetProperty(
    "Cell", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return CellCtor(partOne, args);
    }));

  JsValue cell = globalObject.GetProperty("Cell");
  cell.SetProperty("prototype",
                   globalObject.GetProperty("Form").GetProperty("prototype"));

  JsValue cellPrototype = cell.GetProperty("prototype");

  cellPrototype.SetProperty(
    "GetLocation",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetLocation(partOne, args);
    }));

  cellPrototype.SetProperty(
    "GetFlags", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetFlags(partOne, args);
    }));

  cellPrototype.SetProperty(
    "IsInterior",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return IsInterior(partOne, args);
    }));
}

JsValue CellCtor(std::shared_ptr<PartOne> partOne,
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
    partOne->GetLogger().error("Cell not exists");
    return JsValue::Undefined();
  }

  return JsValue::Undefined();
}

JsValue GetLocation(std::shared_ptr<PartOne> partOne,
                    const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  JsValue locationId = JsValue::Undefined();

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "XLCN") {
        locationId = JsValue::Int(*(uint32_t*)data);
      }
    },
    cache);

  return locationId;
}

JsValue GetFlags(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  JsValue flags = JsValue::Undefined();

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "DATA") {
        flags = JsValue::Array(size / 2);
        for (int i = 0; i < size / 2; i++) {
          flags.SetProperty(i, JsValue::Int(*(uint32_t*)&data[i * 2]));
        }
      }
    },
    cache);

  return flags;
}

JsValue IsInterior(std::shared_ptr<PartOne> partOne,
                   const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  JsValue isInterior = JsValue::Bool(false);

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "DATA") {
        for (int i = 0; i < size / 2; i++) {
          if (*(uint32_t*)&data[i * 2] == 0x0001) {
            isInterior = JsValue::Bool(true);
          }
        }
      }
    },
    cache);

  return isInterior;
}
