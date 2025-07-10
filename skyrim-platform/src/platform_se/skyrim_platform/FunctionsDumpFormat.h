#include <string>

#include "papyrus-vm/Reader.h"
#include "papyrus-vm/Utils.h" // Utils::stricmp

namespace FunctionsDumpFormat {

std::string FindModuleName(uintptr_t moduleBase);

// TODO: consider switching to TypeInfo::TypeAsString (CommonLibSSE-NG)
std::string RawTypeToString(TypeInfo::RawType raw);

struct ValueType
{
  ValueType() = default;

  explicit ValueType(const RE::BSScript::TypeInfo& typeInfo);

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
                            const RE::BSScript::TypeInfo& type_);

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
                    uintptr_t funcOffset);

  void EnrichValueNamesAndTypes(const Object& pexScriptObject);

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
                const std::vector<std::shared_ptr<PexScript>>& pexScripts);

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("types", types);
  }

  std::map<std::string, Type> types;
};
}
