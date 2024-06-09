#include "PapyrusFaction.h"

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
