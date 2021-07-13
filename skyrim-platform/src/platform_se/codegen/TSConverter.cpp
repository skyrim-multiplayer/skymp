
#include "nlohmann/json.hpp"
#include <algorithm>  // For transform
#include <cctype>     // for toupper() and tolower()
#include <filesystem> // For exist()
#include <fstream>    // For ifstream and ofstream
#include <functional> // For std::function
#include <iomanip>    // For setw()
#include <iostream>
#include <iostream> // For stringstream
#include <set>

int main(int argc, char* argv[])
{
  const std::filesystem::path pathToJsonFile = argv[1];
  const std::filesystem::path pathToDefinitionsFile = argv[2];
  const std::filesystem::path pathToTypeScriptFile = argv[3];

  if (!std::filesystem::exists(pathToJsonFile)) {
    std::cout << "Json file: " << pathToJsonFile << " doesn't exits, check it.";
    return 1;
  }
  if (!std::filesystem::exists(pathToDefinitionsFile)) {
    std::cout << "Default papyrus classes file: " << pathToDefinitionsFile
              << " doesn't exits, check it.";
    return 2;
  }

  std::filesystem::create_directories(pathToTypeScriptFile.parent_path());

  std::ifstream input(pathToJsonFile);
  std::ifstream papyrusClasses(pathToDefinitionsFile);
  std::ofstream output(pathToTypeScriptFile);

  enum class Mode
  {
    Module = 0,
    Interface
  };

  const Mode currentMode = Mode::Module; // 'interface' | 'module'
  // TODO: Implement interface mode correctly

  const auto getPrefix = [&currentMode]() -> std::string {
    return currentMode == Mode::Interface ? "export interface SkyrimPlatform "
                                            "{ \n"
                                          : "";
  };
  const auto getPostfix = [&currentMode]() -> std::string {
    return currentMode == Mode::Interface ? "}\n" : "";
  };

  nlohmann::json j;
  input >> j;

  std::string tab = "    ";

  const std::set<std::string> ignored = { "TESModPlatform.Add", "Math",
                                          "MpClientPlugin" };
  std::set<std::string> dumped = {};
  const std::map<std::string, std::string> functionNameOverrides = {
    { "getplayer", "getPlayer" }
  };

  auto prettify = [&](std::string name,
                      std::function<int(int)> func = toupper) -> std::string {
    char firstChar = func(name.at(0));
    name.erase(name.begin());

    // TODO: refactor this please
    auto lowerCaseName = name;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(),
                   lowerCaseName.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto upperCaseName = name;
    std::transform(upperCaseName.begin(), upperCaseName.end(),
                   upperCaseName.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    if ((lowerCaseName == name) || (upperCaseName == name))
      std::transform(name.begin(), name.end(), name.begin(),
                     [](unsigned char c) { return std::tolower(c); });

    return firstChar + name;
  };

  auto parseReturnValue = [&](std::string rawType,
                              std::string objectTypeName) -> std::string {
    switch (rawType)
    {
    case "Int":
      return "number";
      break;
    case "Float":
      return "number";
      break;
    case "Bool":
      return "boolean";
      break;
    case "String":
      return "string";
      break;
    case "IntArray":
      return "number[] | null";
      break;
    case "FloatArray":
      return "number[] | null"
      break;
    case "BoolArray":
      return "boolean[] | null";
      break;
    case "StringArray":
      return "string[] | null"
      break;
    case "None":
      return "void";
      break;
    case "Object":
      return (!objectTypeName.empty() ? prettify(objectTypeName) : "Form") +
        " | null";
      break;
    case "ObjectArray":
      return "PapyrusObject[] | null";
      break;
    default:
      return "";
      break;
    }
  };

  auto dumpFunction = [&](std::string className, nlohmann::json f,
                          bool isGlobal) {
    std::string name = f["name"].get<std::string>();        
    if (ignored.contains(
          (className + "." + name).c_str())) {
      return;
    }

    auto funcName =
      functionNameOverrides.contains(name)
      ? functionNameOverrides.at(name) : name;

    output << tab << (isGlobal ? "static " : "")
           << prettify(funcName, ::tolower);
    // It is no longer important for us in what case the letters were, but then
    // it is more convenient to work with them in lower case
    std::transform(funcName.begin(), funcName.end(), funcName.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    output << "(";
    bool isAddOrRemove = (funcName == "additem" || funcName == "removeitem");

#ifdef _DEBUG
    auto debugFunctionJson = f.dump();
#endif // _DEBUG

    int i = 0; // Hotfix for find first MotionType argument
    for (auto& argument : f.at("arguments")) {
      bool isSetMotioTypeFistArg = ((funcName == "setmotiontype") && (i == 0));

      std::string argType = isSetMotioTypeFistArg
        ? "MotionType"
        : parseReturnValue(
            argument.at("type").at("rawType").get<std::string>(),
            argument.at("type").contains("objectTypeName")
              ? argument.at("type").at("objectTypeName").get<std::string>()
              : "");

      auto argumentName = (argument.at("name").get<std::string>() == "in")
        ? "_in"
        : argument.at("name").get<std::string>();
      output << argumentName << ": " << argType;
      if (i != f.at("arguments").size() - 1) {
        output << ", ";
      }
      i++;
    }

    auto returnType = parseReturnValue(
      f.at("returnType").at("rawType").get<std::string>(),
      f.at("returnType").contains("objectTypeName")
        ? f.at("returnType").at("objectTypeName").get<std::string>()
        : "");

    if (f.at("isLatent").get<bool>()) {
      if (!isAddOrRemove)
        returnType = "Promise<" + returnType + ">";
    }

    output << ")"
           << ": " << returnType << ";\n";
  };

  std::function<void(nlohmann::json)> dumpType =
    [&](nlohmann::json data) -> void {

    std::string name = data.at("name").get<std::string>();
    std::string dataName = data["name"].get<std::string>();
    std::string parent = data["parent"].get<std::string>();

    if (ignored.contains(name) || dumped.contains(name)) {
      return;
    }

    if (data.contains("parent")) {
      dumpType(j["types"].at(parent));
    }

#ifdef _DEBUG
    auto debugTypeJson = data.dump();
    auto debugName = prettify(dataName);
    auto debugParent =
      (data.contains("parent") ? prettify(parent)
                               : "PapyrusObject");
#endif // _DEBUG

    output << "\n// Based on " << prettify(dataName)
           << ".pex\n";
    output << "export declare class "
           << prettify(dataName) << " extends "
           << (data.contains("parent") ? prettify(parent) : "PapyrusObject")
           << "{\n";
    output << tab << "static from(papyrusObject: PapyrusObject | null) : "
           << prettify(dataName) << "| null; \n";

    for (auto& function : data.at("memberFunctions")) {
      dumpFunction(name, function, false);
    }
    for (auto& function : data.at("globalFunctions")) {
      dumpFunction(name, function, true);
    }

    output << "}\n";
    dumped.insert(dataName);
  };

#ifdef _DEBUG
  std::string debugJson = j.dump();
#endif // _DEBUG

  if (!j.at("types").contains("WorldSpace")) {
    j["types"]["WorldSpace"]["parent"] = "Form";
    j["types"]["WorldSpace"]["globalFunctions"] = nlohmann::json::array();
    j["types"]["WorldSpace"]["memberFunctions"] = nlohmann::json::array();
  };

#ifdef _DEBUG
  debugJson = j.dump();
#endif // _DEBUG

  for (auto& typeName : j.at("types").items()) {
    typeName.value()["name"] = typeName.key();
  }

#ifdef _DEBUG
  debugJson = j.dump();
#endif // _DEBUG

  // TODO: optimize i/o files
  output << "\n";
  output << "/* eslint-disable "
            "@typescript-eslint/adjacent-overload-signatures */\n";
  output << "/* eslint-disable @typescript-eslint/no-namespace */\n";
  output << "// Generated automatically. Do not edit.\n";
  output << getPrefix();
  output << "\n";
  output << papyrusClasses.rdbuf();

  for (auto& typeName : j.at("types")) {
    dumpType(typeName);
  }

  output << getPostfix();

  output.close();
  if (!output) {
    std::cout << "TSConverter failed to write to " << pathToTypeScriptFile
              << std::endl;
    return 3;
  }

  return 0;
}
