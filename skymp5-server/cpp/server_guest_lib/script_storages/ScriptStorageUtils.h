#pragma once

#include "papyrus-vm/CIString.h"
#include <set>
#include <string>

namespace ScriptStorageUtils {

std::string GetFileName(const std::string& path);

std::string RemoveExtension(std::string s);

std::set<CIString> GetScriptsInDirectory(const std::string& pexDir);
}
