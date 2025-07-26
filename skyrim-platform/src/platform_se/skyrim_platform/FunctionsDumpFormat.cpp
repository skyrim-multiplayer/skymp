#include "FunctionsDumpFormat.h"
#include "DumpFunctions.h"

#include <psapi.h>
#include <windows.h>

#include "papyrus-vm/Reader.h"
#include "papyrus-vm/Utils.h" // Utils::stricmp

std::string FunctionsDumpFormat::FindModuleName(uintptr_t moduleBase)
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
std::string FunctionsDumpFormat::RawTypeToString(
  RE::BSScript::TypeInfo::RawType raw)
{
  switch (raw) {
    case RE::BSScript::TypeInfo::RawType::kNone:
      return "None";
    case RE::BSScript::TypeInfo::RawType::kObject:
      return "Object";
    case RE::BSScript::TypeInfo::RawType::kString:
      return "String";
    case RE::BSScript::TypeInfo::RawType::kInt:
      return "Int";
    case RE::BSScript::TypeInfo::RawType::kFloat:
      return "Float";
    case RE::BSScript::TypeInfo::RawType::kBool:
      return "Bool";
    case RE::BSScript::TypeInfo::RawType::kNoneArray:
      return "NoneArray";
    case RE::BSScript::TypeInfo::RawType::kObjectArray:
      return "ObjectArray";
    case RE::BSScript::TypeInfo::RawType::kStringArray:
      return "StringArray";
    case RE::BSScript::TypeInfo::RawType::kIntArray:
      return "IntArray";
    case RE::BSScript::TypeInfo::RawType::kFloatArray:
      return "FloatArray";
    case RE::BSScript::TypeInfo::RawType::kBoolArray:
      return "BoolArray";
  }
  return "";
}

FunctionsDumpFormat::ValueType::ValueType(
  const RE::BSScript::TypeInfo& typeInfo)
{
  RE::BSScript::TypeInfo::RawType unmangled = typeInfo.GetUnmangledRawType();

  if (unmangled == TypeInfo::RawType::kObject) {
    objectTypeName = typeInfo.GetTypeInfo()->GetName();
  }

  rawType = RawTypeToString(unmangled);
}

FunctionsDumpFormat::FunctionArgument::FunctionArgument(
  const RE::BSFixedString& name_, const RE::BSScript::TypeInfo& type_)
{
  name = name_;
  type = ValueType(type_);
}

FunctionsDumpFormat::Function::Function(RE::BSScript::IFunction* function,
                                        uintptr_t moduleBase,
                                        uintptr_t funcOffset,
                                        uintptr_t isLongSignature)
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
  useLongSignature = isLongSignature != 0;
  moduleName = FindModuleName(moduleBase);
}

void FunctionsDumpFormat::Function::EnrichValueNamesAndTypes(
  const Object& pexScriptObject)
{
  auto stateIt =
    std::find_if(pexScriptObject.states.begin(), pexScriptObject.states.end(),
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

FunctionsDumpFormat::Root::Root(
  const std::vector<
    std::tuple<std::string, std::string, RE::BSScript::IFunction*, uintptr_t,
               uintptr_t, uintptr_t>>& data,
  const std::vector<std::shared_ptr<PexScript>>& pexScripts)
{
  auto pexScriptIterator = pexScripts.end();
  std::optional<std::string> pexClassName;

  for (auto [className, functionName, function, moduleBase, funcOffset,
             isLongSignature] : data) {
    auto& type = types[className];

    if (pexClassName != className) {
      pexClassName = className;
      pexScriptIterator = std::find_if(
        pexScripts.begin(), pexScripts.end(),
        [&](const std::shared_ptr<PexScript>& pexScript) {
          return !Utils::stricmp(pexScript->source.data(), className.data());
        });
    }

    Function functionDump(function, moduleBase, funcOffset, isLongSignature);

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
