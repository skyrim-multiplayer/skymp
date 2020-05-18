#include "DevApi.h"

#include "InvalidArgumentException.h"
#include "NullPointerException.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include <RE/ConsoleLog.h>

std::shared_ptr<JsEngine>* DevApi::jsEngine = nullptr;
DevApi::NativeExportsMap DevApi::nativeExportsMap;

JsValue DevApi::Require(const JsFunctionArguments& args,
                        std::filesystem::path builtScriptsDir)
{
  auto fileName = args[1].ToString();

  if (fileName.find("..") != std::string::npos)
    throw InvalidArgumentException("fileName", fileName);

  while (!fileName.empty() && (fileName[0] == '/' || fileName[0] == '\\'))
    fileName = { fileName.begin() + 1, fileName.end() };

  std::filesystem::path filePath = builtScriptsDir / (fileName + ".js");

  if (!std::filesystem::exists(filePath))
    throw std::runtime_error("'" + filePath.string() + "' doesn't exist");

  std::ifstream t(filePath);
  if (!t.is_open())
    throw std::runtime_error("Failed to open '" + filePath.string() +
                             "' for reading");

  std::stringstream src;
  src << t.rdbuf();

  if (!jsEngine || !*jsEngine)
    throw NullPointerException("jsEngine");
  auto exports = (**jsEngine).RunScript(src.str(), fileName);

  if (auto& f = DevApi::nativeExportsMap[fileName])
    exports = f(exports);

  return exports;
}

JsValue DevApi::AddNativeExports(const JsFunctionArguments& args)
{
  auto fileName = (std::string)args[1];
  auto exports = args[2];

  if (auto& f = DevApi::nativeExportsMap[fileName])
    exports = f(exports);
  return exports;
}