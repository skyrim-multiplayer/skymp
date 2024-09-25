#include "InventoryApi.h"
#include "CallNativeApi.h"
#include "NullPointerException.h"

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace {

Napi::Value ToJsValue(Napi::Env env, RE::ExtraHealth& extra)
{
  auto res = Napi::Object::New(env);
  res.Set("health", Napi::Number::New(env, extra.health));
  res.Set("type", Napi::String::New(env, "Health"));
  return res;
}

Napi::Value ToJsValue(Napi::Env env, RE::ExtraCount& extra)
{
  auto res = Napi::Object::New(env);
  res.Set("count", Napi::Number::New(env, extra.count));
  res.Set("type", Napi::String::New(env, "Count"));
  return res;
}

Napi::Value ToJsValue(Napi::Env env, RE::ExtraEnchantment& extra)
{
  auto res = Napi::Object::New(env);
  res.Set("enchantmentId",
          Napi::Number::New(
            env, (extra.enchantment ? extra.enchantment->formID : 0)));
  res.Set("maxCharge", extra.charge);
  res.Set("type", Napi::String::New(env, "Enchantment"));
  return res;
}

Napi::Value ToJsValue(Napi::Env env, RE::ExtraCharge& extra)
{
  auto res = Napi::Object::New(env);
  res.Set("charge", Napi::Number::New(env, extra.charge));
  res.Set("type", Napi::String::New(env, "Charge"));
  return res;
}

Napi::Value ToJsValue(Napi::Env env, RE::ExtraTextDisplayData& extra)
{
  auto res = Napi::Object::New(env);
  res.Set("name", Napi::String::New(env, extra.displayName.c_str()));
  res.Set("type", Napi::String::New(env, "TextDisplayData"));
  return res;
}

Napi::Value ToJsValue(Napi::Env env, RE::ExtraSoul& extra)
{
  auto res = Napi::Object::New(env);
  res.Set("soul", Napi::Number::New(env, static_cast<int>(extra.GetType())));
  res.Set("type", Napi::String::New(env, "Soul"));
  return res;
}

Napi::Value ToJsValue(Napi::Env env, RE::ExtraPoison& extra)
{
  auto res = Napi::Object::New(env);
  res.Set("poisonId",
          Napi::Number::New(env, (extra.poison ? extra.poison->formID : 0)));
  res.Set(
    "count",
    Napi::Number::New(env, (reinterpret_cast<RE::ExtraPoison&>(extra).count)));
  res.Set("type", Napi::String::New(env, "Poison"));
  return res;
}

Napi::Value ToJsValue(Napi::Env env, RE::ExtraWorn& extra)
{
  auto res = Napi::Object::New(env);
  res.Set("type", Napi::String::New(env, "Worn"));
  return res;
}

Napi::Value ToJsValue(Napi::Env env, RE::ExtraWornLeft& extra)
{
  auto res = Napi::Object::New(env);
  res.Set("type", Napi::String::New(env, "WornLeft"));
  return res;
}

Napi::Value ToJsValue(Napi::Env env, RE::BSExtraData* extraData)
{
  /**
   * ExtraDataList has GetByType<T> which can be used to avoid casts *
   */
  if (extraData) {
    switch (extraData->GetType()) {
      case RE::ExtraDataType::kHealth:
        return ToJsValue(env, *reinterpret_cast<RE::ExtraHealth*>(extraData));
      case RE::ExtraDataType::kCount:
        return ToJsValue(env, *reinterpret_cast<RE::ExtraCount*>(extraData));
      case RE::ExtraDataType::kEnchantment:
        return ToJsValue(env,
                         *reinterpret_cast<RE::ExtraEnchantment*>(extraData));
      case RE::ExtraDataType::kCharge:
        return ToJsValue(env, *reinterpret_cast<RE::ExtraCharge*>(extraData));
      case RE::ExtraDataType::kTextDisplayData:
        return ToJsValue(
          env, *reinterpret_cast<RE::ExtraTextDisplayData*>(extraData));
      case RE::ExtraDataType::kSoul:
        return ToJsValue(env, *reinterpret_cast<RE::ExtraSoul*>(extraData));
      case RE::ExtraDataType::kPoison:
        return ToJsValue(env, *reinterpret_cast<RE::ExtraPoison*>(extraData));
      case RE::ExtraDataType::kWorn:
        return ToJsValue(env, *reinterpret_cast<RE::ExtraWorn*>(extraData));
      case RE::ExtraDataType::kWornLeft:
        return ToJsValue(env,
                         *reinterpret_cast<RE::ExtraWornLeft*>(extraData));
    }
  }
  return env.Undefined();
}

Napi::Value ToJsValue(Napi::Env env, RE::ExtraDataList* extraList)
{
  if (!extraList) {
    return env.Null();
  }

  std::vector<Napi::Value> jData;

  RE::BSReadLockGuard lock(extraList->_lock);

  for (auto it = extraList->begin(); it != extraList->end(); ++it) {
    auto extra = ToJsValue(env, &(*it));
    if (!extra.IsUndefined()) {
      jData.push_back(extra);
    }
  }

  auto resultArray = Napi::Array::New(env, jData.size());
  for (uint32_t i = 0; i < static_cast<uint32_t>(jData.size()); i++) {
    resultArray.Set(i, jData[i]);
  }
  return resultArray;
}

Napi::Value ToJsValue(Napi::Env env,
                      RE::BSSimpleList<RE::ExtraDataList*>* extendDataList)
{
  if (!extendDataList) {
    return env.Null();
  }

  auto first = extendDataList->begin();
  auto last = extendDataList->end();

  std::vector<Napi::Value> arr;
  for (auto it = first; it != last; ++it) {
    arr.push_back(ToJsValue(env, *it));
  }

  auto resultArray = Napi::Array::New(env, arr.size());

  for (uint32_t i = 0; i < static_cast<uint32_t>(arr.size()); i++) {
    resultArray.Set(i, arr[i]);
  }

  return resultArray;
}
}

Napi::Value InventoryApi::GetExtraContainerChanges(
  const Napi::CallbackInfo& info)
{
  auto objectReferenceId =
    NapiHelper::ExtractUInt32(info[0], "objectReferenceId");

  auto refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(objectReferenceId);
  if (!refr ||
      (refr->formType != RE::FormType::ActorCharacter &&
       refr->formType != RE::FormType::Reference)) {
    return info.Env().Null();
  }

  auto cntChanges = refr->extraList.GetByType<RE::ExtraContainerChanges>();

  if (!cntChanges || !cntChanges->changes || !cntChanges->changes->entryList) {
    return info.Env().Null();
  }

  auto first = cntChanges->changes->entryList->begin();
  auto last = cntChanges->changes->entryList->end();

  std::vector<Napi::Value> res;

  for (auto it = first; it != last; ++it) {
    const auto baseID = (*it)->object ? (*it)->object->formID : 0;

    auto jEntry = Napi::Object::New(info.Env());
    jEntry.Set("countDelta", (*it)->countDelta);
    jEntry.Set("baseId", Napi::Number::New(info.Env(), baseID));
    jEntry.Set("extendDataList", ToJsValue(info.Env(), (*it)->extraLists));

    res.push_back(jEntry);
  }

  auto resultArray = Napi::Array::New(info.Env(), res.size());

  for (uint32_t i = 0; i < static_cast<uint32_t>(res.size()); i++) {
    resultArray.Set(i, res[i]);
  }

  return resultArray;
}

Napi::Value InventoryApi::GetContainer(const Napi::CallbackInfo& info)
{
  auto formID = NapiHelper::ExtractUInt32(info[0], "formId");
  auto form = RE::TESForm::LookupByID<RE::TESObjectREFR>(formID);

  if (!form) {
    return Napi::Array::New(info.Env(), 0);
  }

  auto pContainer = form->As<RE::TESContainer>();

  if (!pContainer) {
    return Napi::Array::New(info.Env(), 0);
  }

  auto res = Napi::Array::New(info.Env(), pContainer->numContainerObjects);

  for (uint32_t i = 0; i < pContainer->numContainerObjects; ++i) {
    auto e = pContainer->containerObjects[i];
    auto jEntry = Napi::Object::New(info.Env());
    jEntry.Set("count", Napi::Number::New(info.Env(), e->count));
    jEntry.Set("baseId",
               Napi::Number::New(info.Env(), e->obj ? e->obj->formID : 0));
    res.Set(i, jEntry);
  }

  return res;
}

namespace {
struct BoundObject
{
  double baseId;
  int count;
  RE::BGSEquipSlot* slot;
};
enum EquipSlot
{
  BothHands = 0x13f45,
  LeftHand = 0x13f43,
  RightHand = 0x13f42
};
}

Napi::Value InventoryApi::SetInventory(const Napi::CallbackInfo& info)
{
  double formId = NapiHelper::ExtractDouble(info[0], "formId");
  Napi::Object inventory = NapiHelper::ExtractObject(info[1], "inv");

  Napi::Array entries =
    NapiHelper::ExtractArray(inventory.Get("entries"), "inv.entries");

  const int size = entries.Length();
  RE::Actor* pActor = RE::TESForm::LookupByID<RE::Actor>(formId);

  if (!pActor) {
    throw NullPointerException("pActor");
  }

  std::vector<BoundObject> objects;

  objects.reserve(size);

  for (int i = 0; i < size; ++i) {
    std::string comment = fmt::format("inv.entries[{}]", i);
    std::string comment1 = fmt::format("inv.entries[{}].baseId", i);
    std::string comment2 = fmt::format("inv.entries[{}].count", i);
    std::string comment3 = fmt::format("inv.entries[{}].worn", i);
    std::string comment4 = fmt::format("inv.entries[{}].wornLeft", i);

    Napi::Object entry =
      NapiHelper::ExtractObject(entries.Get(i), comment.data());
    double baseId =
      NapiHelper::ExtractDouble(entry.Get("baseId"), comment1.data());

    const RE::TESBoundObject* pBoundObject =
      RE::TESForm::LookupByID<RE::TESBoundObject>(baseId);

    if (!pBoundObject) {
      continue;
    }

    int count = NapiHelper::ExtractInt32(entry.Get("count"), comment2.data());

    const bool worn = (!entry.Get("worn").IsUndefined())
      ? NapiHelper::ExtractBoolean(entry.Get("worn"), comment3.data())
      : false;

    const bool wornLeft = (!entry.Get("wornLeft").IsUndefined())
      ? NapiHelper::ExtractBoolean(entry.Get("wornLeft"), comment4.data())
      : false;

    RE::BGSEquipSlot* slot = nullptr;

    if (worn || wornLeft) {
      slot = worn ? static_cast<RE::BGSEquipSlot*>(
                      RE::TESForm::LookupByID(EquipSlot::RightHand))
                  : static_cast<RE::BGSEquipSlot*>(
                      RE::TESForm::LookupByID(EquipSlot::LeftHand));
    }

    objects.push_back({ baseId, count, slot });
  }

  g_nativeCallRequirements.gameThrQ->AddTask([formId, objects](Napi::Env env) {
    RE::Actor* pActor = RE::TESForm::LookupByID<RE::Actor>(formId);

    if (!pActor) {
      return;
    }

    for (auto& object : objects) {
      RE::TESBoundObject* pBoundObject =
        RE::TESForm::LookupByID<RE::TESBoundObject>(object.baseId);

      if (!pBoundObject) {
        continue;
      }

      pActor->AddObjectToContainer(pBoundObject, nullptr, object.count,
                                   nullptr);
      if (object.slot) {
        RE::ActorEquipManager* manager = RE::ActorEquipManager::GetSingleton();
        bool forceEquip = pActor->GetFormID() != 0x14;
        manager->EquipObject(pActor, pBoundObject, nullptr, 1, object.slot,
                             false, forceEquip, false, false);
      }
    }
  });
  return info.Env().Undefined();
}

Napi::Value InventoryApi::CastSpellImmediate(const Napi::CallbackInfo& info)
{
  auto env = info.Env();

  RE::Actor* pActor = RE::TESForm::LookupByID<RE::Actor>(
    NapiHelper::ExtractUInt32(info[0], "formId"));

  const auto castingSource = static_cast<RE::MagicSystem::CastingSource>(
    NapiHelper::ExtractInt32(info[1], "castingSource"));

  const auto formIdSpell =
    reinterpret_cast<RE::MagicItem*>(RE::TESForm::LookupByID(
      NapiHelper::ExtractUInt32(info[2], "formIdSpell")));

  if (!formIdSpell) {
    return env.Undefined();
  }

  auto t = formIdSpell->GetFormType();

  if (t != RE::FormType::Spell && t != RE::FormType::Scroll &&
      t != RE::FormType::Ingredient && t != RE::FormType::AlchemyItem &&
      t != RE::FormType::Enchantment) {
    return env.Undefined();
  }

  const auto formIdTarget = RE::TESForm::LookupByID<RE::TESObjectREFR>(
    NapiHelper::ExtractUInt32(info[3], "formIdTarget"));

  if (!formIdTarget) {
    return env.Undefined();
  }

  if (!pActor) {
    return env.Undefined();
  }

  const auto magicCaster = pActor->GetMagicCaster(castingSource);

  if (!magicCaster) {
    return env.Undefined();
  }

  magicCaster->CastSpellImmediate(formIdSpell, false, formIdTarget, 1.0f,
                                  false, 0.0f, pActor);

  return env.Undefined();
}

void InventoryApi::Register(Napi::Env env, Napi::Object& exports)
{
  exports.Set("getExtraContainerChanges",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(GetExtraContainerChanges)));
  exports.Set(
    "getContainer",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetContainer)));
  exports.Set(
    "setInventory",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetInventory)));
  exports.Set("castSpellImmediate",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(CastSpellImmediate)));
}
