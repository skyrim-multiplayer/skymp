#include "EquipmentBinding.h"
#include "NapiHelper.h"

Napi::Value EquipmentBinding::Get(Napi::Env env, ScampServer& scampServer,
                                  uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  auto& equipment = actor.GetEquipment();
  auto equipmentDump = equipment.ToJson().dump();
  return NapiHelper::ParseJson(env, equipmentDump);
}
