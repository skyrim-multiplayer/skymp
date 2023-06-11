#include "EquipmentBinding.h"
#include "NapiHelper.h"

Napi::Value EquipmentBinding::Get(Napi::Env env, ScampServer& scampServer,
                                  uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  auto& equipmentDump = actor.GetEquipmentAsJson();
  if (!equipmentDump.empty()) {
    return NapiHelper::ParseJson(env, equipmentDump);
  } else {
    return env.Null();
  }
}
