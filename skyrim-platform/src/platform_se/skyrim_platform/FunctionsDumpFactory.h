#pragma once
#include "FunctionsDumpFormat.h"

#include "papyrus-vm/Reader.h"
#include "papyrus-vm/Utils.h" // Utils::stricmp

class FunctionsDumpFactory
{
public:
  static FunctionsDumpFormat::Root Create(
    const std::vector<
      std::tuple<std::string, std::string, RE::BSScript::IFunction*, uintptr_t,
                 uintptr_t, uintptr_t>>& data,
    const std::vector<std::shared_ptr<PexScript>>& pexScripts);

private:
  static std::string FindModuleName(uintptr_t moduleBase);

  // TODO: consider switching to TypeInfo::TypeAsString (CommonLibSSE-NG)
  static std::string RawTypeToString(TypeInfo::RawType raw);

  static FunctionsDumpFormat::ValueType MakeValueType(
    const RE::BSScript::TypeInfo& typeInfo);

  static FunctionsDumpFormat::FunctionArgument MakeFunctionArgument(
    const RE::BSFixedString& name_, const RE::BSScript::TypeInfo& type_);

  static FunctionsDumpFormat::Function MakeFunction(
    RE::BSScript::IFunction* function, uintptr_t moduleBase,
    uintptr_t funcOffset, uintptr_t isLongSignature);

  static void EnrichValueNamesAndTypes(FunctionsDumpFormat::Function& function,
                                       const Object& pexScriptObject);
};
