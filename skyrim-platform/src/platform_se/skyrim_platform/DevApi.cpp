#include "DevApi.h"

#include "InvalidArgumentException.h"
#include "NullPointerException.h"
#include "ReadFile.h"
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

  for (auto& [moduleName, f] : DevApi::nativeExportsMap) {
    if (moduleName == fileName ||
        std::filesystem::path(fileName).filename().string() == moduleName)
      exports = f(exports);
  }

  return exports;
}

namespace {
std::filesystem::path GetPluginPath(std::string pluginName)
{
  return std::filesystem::path("Data/Platform/Plugins") / (pluginName + ".js");
}
}

JsValue DevApi::GetPluginSourceCode(const JsFunctionArguments& args)
{
  // TODO: Support multifile plugins?
  auto pluginName = args[1].ToString();
  return ReadFile(GetPluginPath(pluginName));
}

JsValue DevApi::WritePlugin(const JsFunctionArguments& args)
{
  // TODO: Support multifile plugins?
  auto pluginName = args[1].ToString();
  auto newSources = args[2].ToString();

  auto path = GetPluginPath(pluginName);

  std::ofstream f(path);
  f << newSources;
  f.close();
  if (!f)
    throw std::runtime_error("Failed to write into " + path.string());
  return JsValue::Undefined();
}

JsValue DevApi::GetPlatformVersion(const JsFunctionArguments& args)
{
  return "0.7.0+build3";
}
