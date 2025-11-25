#pragma once
#include <string>
#include <optional>
#include <vector>
#include <map>

namespace FunctionsDumpFormat {

struct ValueType
{
  ValueType() = default;

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

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("types", types);
  }

  std::map<std::string, Type> types;
};
}
