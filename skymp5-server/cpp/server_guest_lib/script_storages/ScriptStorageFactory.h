#pragma once
#include "AssetsScriptStorage.h"
#include "BsaArchiveScriptStorage.h"
#include "CombinedScriptStorage.h"
#include "DirectoryScriptStorage.h"

#include <nlohmann/json.hpp>

class ScriptStorageFactory
{
public:
  static std::shared_ptr<IScriptStorage> Create(nlohmann::json serverSettings);
};
