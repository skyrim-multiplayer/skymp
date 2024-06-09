#include "PapyrusSkymp.h"

#include "MpActor.h"
#include "script_objects/MpFormGameObject.h"

VarValue PapyrusSkymp::SetDefaultActor(VarValue self,
                                       const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 1) {
    policy->SetDefaultActor(self.GetMetaStackId(),
                            GetFormPtr<MpActor>(arguments[0]));
  }

  return VarValue::None();
}

VarValue PapyrusSkymp::GetFaction(VarValue self,
                                  const std::vector<VarValue>& arguments)
{
  if (arguments.empty()) {
    spdlog::error("Faction.GetFaction - at least one argument expected");
    return VarValue::None();
  }

  CIString factionName = arguments[0].GetType() == VarValue::kType_String
    ? static_cast<const char*>(arguments[0])
    : "";

  for (auto it = factions.rbegin(); it != factions.rend(); ++it) {
    for (auto itFaction = (*it)->begin(); itFaction != (*it)->end();
         ++itFaction) {
      auto faction = reinterpret_cast<const espm::FACT*>(*itFaction);
      WorldState* worldState = policy->GetWorldState();
      CIString otherName = faction->GetEditorId(worldState->GetEspmCache());

      if (otherName == factionName) {
        return VarValue(std::make_shared<EspmGameObject>(
          worldState->GetEspm().GetBrowser().LookupById(faction->GetId())));
      }
    }
  }

  return VarValue::None();
}
