#include "PapyrusFaction.h"

VarValue PapyrusFaction::GetFaction(VarValue self,
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
      WorldState* worldState = compatibilityPolicy->GetWorldState();
      CIString otherName = faction->GetEditorId(worldState->GetEspmCache());

      if (otherName == factionName) {
        return VarValue(std::make_shared<EspmGameObject>(
          worldState->GetEspm().GetBrowser().LookupById(faction->GetId())));
      }
    }
  }

  return VarValue::None();
}

VarValue PapyrusFaction::GetReaction(VarValue self,
                                     const std::vector<VarValue>& arguments)
{
  if (arguments.empty()) {
    spdlog::error("Faction.GetReaction - at least one argument expected");
    return VarValue(-1);
  }

  if (arguments[0].GetType() != VarValue::kType_Object) {
    spdlog::error("Faction.GetReaction - wrong object type passed");
    return VarValue(-1);
  }

  espm::LookupResult faction = GetRecordPtr(self);
  espm::LookupResult otherFaction = GetRecordPtr(arguments[0]);

  WorldState* worldState = compatibilityPolicy->GetWorldState();

  espm::FACT::Data otherFactionData =
    espm::Convert<espm::FACT>(otherFaction.rec)
      ->GetData(worldState->GetEspmCache());

  int reaction = -1;
  for (const auto& interfactionRelation :
       otherFactionData.interfactionRelations) {
    if (interfactionRelation.factionFormId == faction.rec->GetId()) {
      reaction = static_cast<int>(interfactionRelation.combat);
    }
  }

  return VarValue(reaction);
}
