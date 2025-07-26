#include "DumpFunctions.h"
#include "GetNativeFunctionAddr.h"
#include "Hooks.h"
#include "NullPointerException.h"
#include "archives/JsonOutputArchive.h"
#include "papyrus-vm/Reader.h"
#include "papyrus-vm/Utils.h" // Utils::stricmp
#include <algorithm>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>

#include "FunctionsDumpFormat.h"

void DumpFunctions::Run()
{
  try {
    RunImpl(Hooks::GetBoundNatives());
  } catch (std::exception& e) {
    if (auto c = RE::ConsoleLog::GetSingleton())
      c->Print("DumpFunctions::Run: %s", e.what());
  }
}

void DumpFunctions::RunImpl(
  const std::vector<
    std::tuple<std::string, std::string, RE::BSScript::IFunction*, uintptr_t,
               uintptr_t, uintptr_t>>& data)
{
  std::vector<std::string> scriptsPaths;
  auto scriptsDir = std::filesystem::path("Data\\Scripts");
  for (auto& entry : std::filesystem::directory_iterator(scriptsDir)) {
    if (!entry.is_directory() && !entry.is_symlink()) {
      scriptsPaths.push_back(entry.path().string());
    }
  }
  Reader pexReader(scriptsPaths);
  std::vector<std::shared_ptr<PexScript>> pexScripts =
    pexReader.GetSourceStructures();

  FunctionsDumpFormat::Root root(data, pexScripts);

  JsonOutputArchive archive;
  root.Serialize(archive);
  {
    std::filesystem::path dir = "Data\\Platform\\Output";
    std::filesystem::create_directories(dir);
    auto p = dir / "FunctionsDump.txt";
    std::ofstream(p) << archive.j.dump(2);
  }
}
