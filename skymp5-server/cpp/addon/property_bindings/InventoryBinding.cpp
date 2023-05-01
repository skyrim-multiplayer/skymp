#include "InventoryBinding.h"
#include "NapiHelper.h"

Napi::Value InventoryBinding::Get(Napi::Env env, ScampServer& scampServer,
                                  uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto refr = partOne->worldState.Get<MpObjectReference>(formId);
  return NapiHelper::ParseJson(env, refr->GetInventory().ToJson());
}

void InventoryBinding::Set(Napi::Env env, ScampServer& scampServer,
                           uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto refr = partOne->worldState.Get<MpObjectReference>(formId);
  if (newValue.IsObject()) {
    auto inventoryDump = NapiHelper::Stringify(env, newValue);
    simdjson::dom::parser p;
    auto inventory = Inventory::FromJson(p.parse(inventoryDump));
    refr->SetInventory(inventory);
  } else {
    refr->SetInventory(Inventory());
  }
}
