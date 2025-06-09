#include "WornHasKeyword.h"

#include "script_classes/PapyrusActor.h"
#include "script_objects/EspmGameObject.h"

const char* ConditionFunctions::WornHasKeyword::GetName() const
{
  return "WornHasKeyword";
}

uint16_t ConditionFunctions::WornHasKeyword::GetFunctionIndex() const
{
  return 682;
}

float ConditionFunctions::WornHasKeyword::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2)
{
  auto& br = actor.GetParent()->GetEspm().GetBrowser();
  PapyrusActor papyrusActor;
  auto aKeyword =
    VarValue(std::make_shared<EspmGameObject>(br.LookupById(parameter1)));

  VarValue res = papyrusActor.WornHasKeyword(actor.ToVarValue(), { aKeyword });
  bool resBool = static_cast<bool>(res);
  if (resBool) {
    return 1.0f;
  }
  return 0.f;
}
