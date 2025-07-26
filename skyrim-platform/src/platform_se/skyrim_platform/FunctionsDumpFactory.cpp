#include "FunctionsDumpFactory.h"
#include "DumpFunctions.h"

#include <psapi.h>
#include <windows.h>

#include "papyrus-vm/Reader.h"
#include "papyrus-vm/Utils.h" // Utils::stricmp

std::string FunctionsDumpFactory::FindModuleName(uintptr_t moduleBase)
{
  HMODULE hMods[1024];
  DWORD cbNeeded;
  HANDLE hProcess = GetCurrentProcess();

  if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
      if ((uintptr_t)hMods[i] == moduleBase) {
        // TODO: consider GetModuleFileNameExA till it stops truncate. afaik
        // there is no other way to get szModName buffer size before allocating
        // it. MAX_PATH is not good for post Windows 10 systems.
        char szModName[8912];
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
std::string FunctionsDumpFactory::RawTypeToString(
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

FunctionsDumpFormat::ValueType FunctionsDumpFactory::MakeValueType(
  const RE::BSScript::TypeInfo& typeInfo)
{
  FunctionsDumpFormat::ValueType result;

  RE::BSScript::TypeInfo::RawType unmangled = typeInfo.GetUnmangledRawType();

  if (unmangled == TypeInfo::RawType::kObject) {
    result.objectTypeName = typeInfo.GetTypeInfo()->GetName();
  }

  result.rawType = RawTypeToString(unmangled);

  return result;
}

FunctionsDumpFormat::FunctionArgument
FunctionsDumpFactory::MakeFunctionArgument(const RE::BSFixedString& name_,
                                           const RE::BSScript::TypeInfo& type_)
{
  FunctionsDumpFormat::FunctionArgument result;
  result.name = name_;
  result.type = MakeValueType(type_);
  return result;
}

FunctionsDumpFormat::Function FunctionsDumpFactory::MakeFunction(
  RE::BSScript::IFunction* function, uintptr_t moduleBase,
  uintptr_t funcOffset, uintptr_t isLongSignature)
{
  FunctionsDumpFormat::Function result;

  auto nativeFunction =
    reinterpret_cast<RE::BSScript::NF_util::NativeFunctionBase*>(function);

  uint32_t paramCount = function->GetParamCount();
  result.arguments.resize(paramCount);

  for (uint32_t i = 0; i < paramCount; ++i) {
    RE::BSFixedString nameOut;
    RE::BSScript::TypeInfo typeOut;
    function->GetParam(i, nameOut, typeOut);
    result.arguments[i] = MakeFunctionArgument(nameOut, typeOut);
  }

  result.isLatent = nativeFunction->GetIsLatent();
  result.name = nativeFunction->GetName();
  result.offset = funcOffset;
  result.returnType = MakeValueType(nativeFunction->GetReturnType());
  result.useLongSignature = isLongSignature != 0;
  result.moduleName = FindModuleName(moduleBase);

  return result;
}

void FunctionsDumpFactory::EnrichValueNamesAndTypes(
  FunctionsDumpFormat::Function& function, const Object& pexScriptObject)
{
  auto stateIt =
    std::find_if(pexScriptObject.states.begin(), pexScriptObject.states.end(),
                 [&](const auto& state) { return state.name.empty(); });
  if (stateIt == pexScriptObject.states.end()) {
    throw std::runtime_error("Unable to find state in pex object");
  }

  auto funcIt = std::find_if(stateIt->functions.begin(),
                             stateIt->functions.end(), [&](const auto& pexF) {
                               return !Utils::stricmp(pexF.name.data(),
                                                      function.name.data());
                             });
  if (funcIt == stateIt->functions.end()) {
    throw std::runtime_error("Unable to find " + function.name + " in pex");
  }

  // Enrich return type
  function.returnType.pexTypeName = funcIt->function.returnType;

  // Enrich arguments
  size_t n =
    std::min(funcIt->function.params.size(), function.arguments.size());
  for (size_t i = 0; i < n; ++i) {
    function.arguments.at(i).name = funcIt->function.params[i].name;
    function.arguments.at(i).type.pexTypeName =
      funcIt->function.params[i].type;
  }
}

FunctionsDumpFormat::Root FunctionsDumpFactory::Create(
  const std::vector<
    std::tuple<std::string, std::string, RE::BSScript::IFunction*, uintptr_t,
               uintptr_t, uintptr_t>>& data,
  const std::vector<std::shared_ptr<PexScript>>& pexScripts)
{
  FunctionsDumpFormat::Root result;

  auto pexScriptIterator = pexScripts.end();
  std::optional<std::string> pexClassName;

  for (auto [className, functionName, function, moduleBase, funcOffset,
             isLongSignature] : data) {
    auto& type = result.types[className];

    if (pexClassName != className) {
      pexClassName = className;
      pexScriptIterator = std::find_if(
        pexScripts.begin(), pexScripts.end(),
        [&](const std::shared_ptr<PexScript>& pexScript) {
          return !Utils::stricmp(pexScript->source.data(), className.data());
        });
    }

    FunctionsDumpFormat::Function functionDump =
      MakeFunction(function, moduleBase, funcOffset, isLongSignature);

    if (pexScriptIterator != pexScripts.end()) {
      auto& pex = (*pexScriptIterator);
      auto classNameCstr = className.data();
      auto pexObjectIterator = std::find_if(
        pex->objectTable.begin(), pex->objectTable.end(),
        [&](const auto& obj) {
          return !Utils::stricmp(obj.NameIndex.data(), classNameCstr);
        });
      if (pexObjectIterator != pex->objectTable.end()) {
        EnrichValueNamesAndTypes(functionDump, *pexObjectIterator);
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

  return result;
}
