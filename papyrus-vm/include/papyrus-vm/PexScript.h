#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "DebugInfo.h"
#include "Object.h"
#include "ScriptHeader.h"
#include "StringTable.h"
#include "UserFlag.h"

struct PexScript
{
  // Copying PexScript breaks VarValues with strings
  PexScript() = default;
  PexScript(const PexScript&) = delete;
  PexScript& operator=(const PexScript&) = delete;

  struct Lazy
  {
    std::string source;
    std::function<std::shared_ptr<PexScript>()> fn;
  };

  ScriptHeader header;
  StringTable stringTable;
  DebugInfo debugInfo;
  std::vector<UserFlag> userFlagTable;
  std::vector<Object> objectTable;

  std::string source;
  std::string user;
  std::string machine;
};
