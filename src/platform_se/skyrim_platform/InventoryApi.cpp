#include "InventoryApi.h"
#include "NullPointerException.h"
#include <RE/ExtraPoison.h>
#include <RE/ExtraWorn.h>
#include <RE/ExtraWornLeft.h>
#include <set>
#include <skse64/GameExtraData.h>
#include <skse64/GameRTTI.h>
#include <skse64/GameReferences.h>
#include <skse64/PapyrusObjectReference.h>

namespace {

JsValue ToJsValue(ExtraHealth& extra)
{
  auto res = JsValue::Object();
  res.SetProperty("health", extra.health);
  res.SetProperty("type", "Health");
  return res;
}

JsValue ToJsValue(ExtraCount& extra)
{
  auto res = JsValue::Object();
  res.SetProperty("count", static_cast<int>(extra.count));
  res.SetProperty("type", "Count");
  return res;
}

JsValue ToJsValue(ExtraEnchantment& extra)
{
  auto res = JsValue::Object();
  res.SetProperty(
    "enchantmentId",
    static_cast<double>(extra.enchant ? extra.enchant->formID : 0));
  res.SetProperty("maxCharge", extra.maxCharge);
  res.SetProperty("type", "Enchantment");
  return res;
}

JsValue ToJsValue(ExtraCharge& extra)
{
  auto res = JsValue::Object();
  res.SetProperty("charge", extra.charge);
  res.SetProperty("type", "Charge");
  return res;
}

JsValue ToJsValue(ExtraTextDisplayData& extra)
{
  auto res = JsValue::Object();
  res.SetProperty("name", extra.name.data);
  res.SetProperty("type", "TextDisplayData");
  return res;
}

JsValue ToJsValue(ExtraSoul& extra)
{
  auto res = JsValue::Object();
  res.SetProperty("soul", static_cast<int>(extra.count));
  res.SetProperty("type", "Soul");
  return res;
}

JsValue ToJsValue(ExtraPoison& extra)
{
  auto res = JsValue::Object();
  res.SetProperty(
    "poisonId", static_cast<double>(extra.poison ? extra.poison->formID : 0));
  res.SetProperty(
    "count",
    static_cast<double>(reinterpret_cast<RE::ExtraPoison&>(extra).count));
  res.SetProperty("type", "Poison");
  return res;
}

JsValue ToJsValue(ExtraWorn& extra)
{
  auto res = JsValue::Object();
  res.SetProperty("type", "Worn");
  return res;
}

JsValue ToJsValue(ExtraWornLeft& extra)
{
  auto res = JsValue::Object();
  res.SetProperty("type", "WornLeft");
  return res;
}

JsValue ToJsValue(BSExtraData* extraData)
{
  if (extraData) {
    switch (extraData->GetType()) {
      case kExtraData_Health:
        return ToJsValue(*reinterpret_cast<ExtraHealth*>(extraData));
      case kExtraData_Count:
        return ToJsValue(*reinterpret_cast<ExtraCount*>(extraData));
      case kExtraData_Enchantment:
        return ToJsValue(*reinterpret_cast<ExtraEnchantment*>(extraData));
      case kExtraData_Charge:
        return ToJsValue(*reinterpret_cast<ExtraCharge*>(extraData));
      case kExtraData_TextDisplayData:
        return ToJsValue(*reinterpret_cast<ExtraTextDisplayData*>(extraData));
      case kExtraData_Soul:
        return ToJsValue(*reinterpret_cast<ExtraSoul*>(extraData));
      case kExtraData_Poison:
        return ToJsValue(*reinterpret_cast<ExtraPoison*>(extraData));
      case kExtraData_Worn:
        return ToJsValue(*reinterpret_cast<ExtraWorn*>(extraData));
      case kExtraData_WornLeft:
        return ToJsValue(*reinterpret_cast<ExtraWornLeft*>(extraData));
    }
  }
  return JsValue::Undefined();
}

JsValue ToJsValue(BaseExtraList* extraList)
{
  if (!extraList)
    return JsValue::Null();

  std::vector<JsValue> jData;

  BSReadLocker lock(&extraList->m_lock);

  for (auto data = extraList->m_data; data != nullptr; data = data->next) {
    auto extra = ToJsValue(data);
    if (extra.GetType() != JsValue::Type::Undefined)
      jData.push_back(extra);
  }

  return jData;
}

JsValue ToJsValue(tList<BaseExtraList>* extendDataList)
{
  if (!extendDataList)
    return JsValue::Null();

  auto arr = JsValue::Array(extendDataList->Count());
  for (uint32_t i = 0; i < extendDataList->Count(); ++i) {
    auto baseExtraList = extendDataList->GetNthItem(i);
    arr.SetProperty(JsValue::Int(i), ToJsValue(baseExtraList));
  }
  return arr;
}
}

JsValue InventoryApi::GetExtraContainerChanges(const JsFunctionArguments& args)
{
  auto objectReferenceId = static_cast<double>(args[1]);

  auto refr =
    reinterpret_cast<TESObjectREFR*>(LookupFormByID(objectReferenceId));
  if (!refr ||
      (refr->formType != kFormType_Character &&
       refr->formType != kFormType_Reference)) {
    return JsValue::Null();
  }

  auto extraCntainerChanges = reinterpret_cast<ExtraContainerChanges*>(
    refr->extraData.GetByType(kExtraData_ContainerChanges));
  if (!extraCntainerChanges || !extraCntainerChanges->data)
    return JsValue::Null();

  tList<InventoryEntryData>* objList = extraCntainerChanges->data->objList;
  if (!objList)
    throw NullPointerException("objList");

  auto res = JsValue::Array(objList->Count());
  for (uint32_t i = 0; i < objList->Count(); ++i) {
    auto entry = objList->GetNthItem(static_cast<int>(i));
    if (!entry)
      continue;

    const auto baseId = entry->type ? entry->type->formID : 0;

    auto jEntry = JsValue::Object();
    jEntry.SetProperty("countDelta", entry->countDelta);
    jEntry.SetProperty("baseId", JsValue::Double(baseId));
    jEntry.SetProperty("extendDataList", ToJsValue(entry->extendDataList));
    res.SetProperty(JsValue::Int(i), jEntry);
  }
  return res;
}

JsValue InventoryApi::GetContainer(const JsFunctionArguments& args)
{
  auto formId = static_cast<double>(args[1]);
  auto form = reinterpret_cast<TESObjectREFR*>(LookupFormByID(formId));

  if (!form)
    return JsValue::Array(0);

  TESContainer* pContainer = DYNAMIC_CAST(form, TESForm, TESContainer);

  if (!pContainer)
    return JsValue::Array(0);

  auto res = JsValue::Array(pContainer->numEntries);
  for (int i = 0; i < static_cast<int>(pContainer->numEntries); ++i) {
    auto e = pContainer->entries[i];

    auto jEntry = JsValue::Object();
    jEntry.SetProperty("count", JsValue::Double(e->count));
    jEntry.SetProperty("baseId",
                       JsValue::Double(e->form ? e->form->formID : 0));
    res.SetProperty(i, jEntry);
  }
  return res;
}