#include "PapyrusFaction.h"

VarValue PapyrusFaction::GetReaction(VarValue self,
                                     const std::vector<VarValue>& arguments)
{
  if (arguments.empty()) {
    spdlog::error("Faction.GetReaction - at least one argument expected");
    return VarValue(-1);
  }

  espm::LookupResult faction = GetRecordPtr(self);
  espm::LookupResult otherFaction = GetRecordPtr(arguments[0]);

  WorldState* worldState = compatibilityPolicy->GetWorldState();

  espm::FACT::Data ourFactionData = espm::Convert<espm::FACT>(faction.rec)
                                      ->GetData(worldState->GetEspmCache());

  int reaction = -1;
  for (const auto& interfactionRelation :
       ourFactionData.interfactionRelations) {
    if (faction.ToGlobalId(interfactionRelation.factionFormId) ==
        otherFaction.ToGlobalId(otherFaction.rec->GetId())) {
      reaction = static_cast<int>(interfactionRelation.combat);
    }
  }

  return VarValue(reaction);
}
