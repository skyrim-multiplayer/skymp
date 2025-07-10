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

#include <psapi.h>
#include <windows.h>

namespace FunctionsDumpFormat {

std::string FindModuleName(uintptr_t moduleBase)
{
  HMODULE hMods[1024];
  DWORD cbNeeded;
  HANDLE hProcess = GetCurrentProcess();

  if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
      if ((uintptr_t)hMods[i] == moduleBase) {
        char szModName[MAX_PATH];
        if (GetModuleFileNameExA(hProcess, hMods[i], szModName,
                                 sizeof(szModName))) {
          return std::string(szModName);
        }
      }
    }
  }

  return std::string();
}

// TODO: consider switching to TypeInfo::TypeAsString (CommonLibSSE-NG)
std::string RawTypeToString(TypeInfo::RawType raw)
{
  switch (raw) {
    case TypeInfo::RawType::kNone:
      return "None";
    case TypeInfo::RawType::kObject:
      return "Object";
    case TypeInfo::RawType::kString:
      return "String";
    case TypeInfo::RawType::kInt:
      return "Int";
    case TypeInfo::RawType::kFloat:
      return "Float";
    case TypeInfo::RawType::kBool:
      return "Bool";
    case TypeInfo::RawType::kNoneArray:
      return "NoneArray";
    case TypeInfo::RawType::kObjectArray:
      return "ObjectArray";
    case TypeInfo::RawType::kStringArray:
      return "StringArray";
    case TypeInfo::RawType::kIntArray:
      return "IntArray";
    case TypeInfo::RawType::kFloatArray:
      return "FloatArray";
    case TypeInfo::RawType::kBoolArray:
      return "BoolArray";
  }
  return "";
}

struct ValueType
{
  ValueType() = default;

  explicit ValueType(const RE::BSScript::TypeInfo& typeInfo)
  {
    RE::BSScript::TypeInfo::RawType unmangled = typeInfo.GetUnmangledRawType();

    if (unmangled == TypeInfo::RawType::kObject) {
      objectTypeName = typeInfo.GetTypeInfo()->GetName();
    }

    rawType = RawTypeToString(unmangled);
  }

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("objectTypeName", objectTypeName)
      .Serialize("rawType", rawType)
      .Serialize("pexTypeName", pexTypeName);
  }

  std::optional<std::string> objectTypeName;
  std::optional<std::string> pexTypeName;
  std::string rawType;
};

struct FunctionArgument
{
  FunctionArgument() = default;

  explicit FunctionArgument(const RE::BSFixedString& name_,
                            const RE::BSScript::TypeInfo& type_)
  {
    name = name_;
    type = ValueType(type_);
  }

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("name", name).Serialize("type", type);
  }

  std::string name;
  ValueType type;
};

struct Function
{
  Function() = default;

  explicit Function(RE::BSScript::IFunction* function, uintptr_t moduleBase,
                    uintptr_t funcOffset)
  {
    auto nativeFunction =
      reinterpret_cast<RE::BSScript::NF_util::NativeFunctionBase*>(function);

    uint32_t paramCount = function->GetParamCount();
    arguments.resize(paramCount);

    for (uint32_t i = 0; i < paramCount; ++i) {
      RE::BSFixedString nameOut;
      RE::BSScript::TypeInfo typeOut;
      function->GetParam(i, nameOut, typeOut);
      arguments[i] = FunctionArgument(nameOut, typeOut);
    }

    isLatent = nativeFunction->GetIsLatent();
    name = nativeFunction->GetName();
    offset = funcOffset;
    returnType = ValueType(nativeFunction->GetReturnType());
    useLongSignature = false; // TODO
    moduleName = FindModuleName(moduleBase);
  }

  void EnrichValueNamesAndTypes(const Object& pexScriptObject)
  {
    auto stateIt = std::find_if(
      pexScriptObject.states.begin(), pexScriptObject.states.end(),
      [&](const auto& state) { return state.name.empty(); });
    if (stateIt == pexScriptObject.states.end()) {
      throw std::runtime_error("Unable to find state in pex object");
    }

    auto funcIt =
      std::find_if(stateIt->functions.begin(), stateIt->functions.end(),
                   [&](const auto& pexF) {
                     return !Utils::stricmp(pexF.name.data(), name.data());
                   });
    if (funcIt == stateIt->functions.end()) {
      throw std::runtime_error("Unable to find " + name + " in pex");
    }

    // Enrich return type
    returnType.pexTypeName = funcIt->function.returnType;

    // Enrich arguments
    size_t n = std::min(funcIt->function.params.size(), arguments.size());
    for (size_t i = 0; i < n; ++i) {
      arguments.at(i).name = funcIt->function.params[i].name;
      arguments.at(i).type.pexTypeName = funcIt->function.params[i].type;
    }
  }

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("arguments", arguments)
      .Serialize("isLatent", isLatent)
      .Serialize("name", name)
      .Serialize("offset", offset)
      .Serialize("returnType", returnType)
      .Serialize("useLongSignature", useLongSignature)
      .Serialize("moduleName", moduleName);
  }

  std::vector<FunctionArgument> arguments;
  bool isLatent = false;
  std::string name;
  uint32_t offset = 0;
  ValueType returnType;
  bool useLongSignature = false;
  std::string moduleName;
};

struct Type
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("globalFunctions", globalFunctions)
      .Serialize("memberFunctions", memberFunctions)
      .Serialize("parent", parent);
  }

  std::vector<Function> globalFunctions;
  std::vector<Function> memberFunctions;
  std::string parent;
};

struct Root
{
  Root() = default;

  explicit Root(const std::vector<std::tuple<std::string, std::string,
                                             RE::BSScript::IFunction*,
                                             uintptr_t, uintptr_t>>& data,
                const std::vector<std::shared_ptr<PexScript>>& pexScripts)
  {
    auto pexScriptIterator = pexScripts.end();
    std::optional<std::string> pexClassName;

    for (auto [className, functionName, function, moduleBase, funcOffset] :
         data) {
      auto& type = types[className];

      if (pexClassName != className) {
        pexClassName = className;
        pexScriptIterator = std::find_if(
          pexScripts.begin(), pexScripts.end(),
          [&](const std::shared_ptr<PexScript>& pexScript) {
            return !Utils::stricmp(pexScript->source.data(), className.data());
          });
      }

      Function functionDump(function, moduleBase, funcOffset);

      if (pexScriptIterator != pexScripts.end()) {
        auto& pex = (*pexScriptIterator);
        auto classNameCstr = className.data();
        auto pexObjectIterator = std::find_if(
          pex->objectTable.begin(), pex->objectTable.end(),
          [&](const auto& obj) {
            return !Utils::stricmp(obj.NameIndex.data(), classNameCstr);
          });
        if (pexObjectIterator != pex->objectTable.end()) {
          functionDump.EnrichValueNamesAndTypes(*pexObjectIterator);
        } else {
          throw std::runtime_error(
            "pexObjectIterator was pex->objectTable.end()");
        }
      }

      if (function->GetIsStatic()) {
        type.globalFunctions.push_back(functionDump);
      } else {
        type.memberFunctions.push_back(functionDump);
      }
    }
  }

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("types", types);
  }

  std::map<std::string, Type> types;
};
}

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
  const std::vector<std::tuple<
    std::string, std::string, RE::BSScript::IFunction*, uintptr_t, uintptr_t>>&
    data)
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

  std::filesystem::path dir = "Data\\Platform\\Output";
  std::filesystem::create_directories(dir);
  auto p = dir / "FunctionsDump.txt";
  std::ofstream(p) << archive.j.dump(2);
}
