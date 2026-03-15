#include "InventoryBinding.h"
#include "NapiHelper.h"

Napi::Value InventoryBinding::Get(Napi::Env env, ScampServer& scampServer,
                                  uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  return NapiHelper::ParseJson(env, refr.GetInventory().ToJson());
}

void InventoryBinding::Set(Napi::Env env, ScampServer& scampServer,
                           uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  if (newValue.IsObject()) {
    auto inventoryDump = NapiHelper::Stringify(env, newValue);
    nlohmann::json j = nlohmann::json::parse(inventoryDump);
    auto inventory = Inventory::FromJson(j);
    refr.SetInventory(inventory);
  } else {
    refr.SetInventory(Inventory());
  }
}
