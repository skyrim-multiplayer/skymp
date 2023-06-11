#pragma once
#include <map>
#include <memory>
#include <string>

class ScriptVariablesHolder;

struct ScriptState
{
  std::map<std::string, std::shared_ptr<ScriptVariablesHolder>> varHolders;
};
