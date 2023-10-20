#pragma once
#include "IPapyrusCompatibilityPolicy.h"
#include <memory>

class WorldState;

class PapyrusCompatibilityPolicyFactory
{
public:
  static std::shared_ptr<IPapyrusCompatibilityPolicy> Create(
    WorldState* worldState);
};
