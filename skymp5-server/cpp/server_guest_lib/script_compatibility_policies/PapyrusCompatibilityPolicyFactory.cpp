#include "PapyrusCompatibilityPolicyFactory.h"

#include "HeuristicPolicy.h"

std::shared_ptr<IPapyrusCompatibilityPolicy>
PapyrusCompatibilityPolicyFactory::Create(WorldState* worldState)
{
  return std::make_shared<HeuristicPolicy>(worldState);
}
