#pragma once
#include "papyrus-vm/CIString.h"
#include <cstdint>
#include <set>
#include <vector>

class IScriptStorage
{
public:
  virtual ~IScriptStorage() = default;

  virtual std::vector<uint8_t> GetScriptPex(const char* scriptName) = 0;

  virtual const std::set<CIString>& ListScripts(bool forceReloadScripts) = 0;
};
