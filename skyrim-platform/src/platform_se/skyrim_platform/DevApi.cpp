#include "DevApi.h"
#include "FileUtils.h"
#include "InvalidArgumentException.h"
#include "NullPointerException.h"
#include "PapyrusTESModPlatform.h"
#include "Settings.h"
#include "Validators.h"

std::shared_ptr<JsEngine> DevApi::jsEngine = nullptr;
DevApi::NativeExportsMap DevApi::nativeExportsMap;

namespace {
bool CreateDirectoryRecursive(const std::string& dirName, std::error_code& err)
{
  err.clear();
  if (!std::filesystem::create_directories(dirName, err)) {
    if (std::filesystem::exists(dirName)) {
      // The folder already exists:
      err.clear();
      return true;
    }
    return false;
  }
  return true;
}
}

JsValue DevApi::Require(
  const JsFunctionArguments& args,
  const std::vector<std::filesystem::path>& pluginLoadDirectories)
{
  auto fileName = args[1].ToString();

  if (!ValidateRelativePath(fileName)) {
    throw InvalidArgumentException("fileName", fileName);
  }

  for (const std::filesystem::path& dir : pluginLoadDirectories) {
    std::filesystem::path filePath = dir / (fileName + ".js");

    if (!std::filesystem::exists(filePath)) {
      continue; // Throws in the end of the function if nothing found
    }

    std::ifstream t(filePath);
    if (!t.is_open()) {
      throw std::runtime_error("Failed to open '" + filePath.string() +
                               "' for reading");
    }

    std::stringstream src;
    src << t.rdbuf();

    if (!jsEngine) {
      throw NullPointerException("jsEngine");
    }
    auto exports = jsEngine->RunScript(src.str(), fileName);

    if (auto& f = DevApi::nativeExportsMap[fileName]) {
      exports = f(exports);
    }

    return exports;
  }

  throw std::runtime_error("'" + fileName + "' doesn't exist");
}

JsValue DevApi::AddNativeExports(const JsFunctionArguments& args)
{
  auto fileName = static_cast<std::string>(args[1]);
  auto exports = args[2];

  for (auto& [moduleName, f] : DevApi::nativeExportsMap) {
    if (moduleName == fileName ||
        std::filesystem::path(fileName).filename().string() == moduleName)
      exports = f(exports);
  }

  return exports;
}

namespace {
std::filesystem::path GetPluginPath(const std::string& pluginName,
                                    std::optional<std::string> folderOverride)
{
  if (!ValidateFilename(pluginName, /*allowDots*/ false)) {
    throw InvalidArgumentException("pluginName", pluginName);
  }

  // Folder override is alowed to be not in list of plugin folders
  // In this case it will be writable, but SkyrimPlatform will not monitor and
  // load plugins from it.
  if (folderOverride) {
    if (!ValidateFilename(folderOverride->data(), /*allowDots*/ false)) {
      throw InvalidArgumentException("folderOverride", *folderOverride);
    }

    return std::filesystem::path("Data/Platform") / *folderOverride /
      (pluginName + ".js");
  }

  auto pluginFolders = Settings::GetPlatformSettings()->GetPluginFolders();

  if (!pluginFolders) {
    throw NullPointerException("pluginFolders");
  }

  if (pluginFolders->empty()) {
    throw std::runtime_error("No plugin folders found");
  }

  auto folder = pluginFolders->front();

  return folder / (pluginName + ".js");
}
}

JsValue DevApi::GetPluginSourceCode(const JsFunctionArguments& args)
{
  // TODO: Support multifile plugins?
  auto pluginName = args[1].ToString();

  std::optional<std::string> overrideFolder;
  if (args.GetSize() >= 3) {
    auto t = args[2].GetType();
    if (t != JsValue::Type::Undefined && t != JsValue::Type::Null) {
      overrideFolder = args[2].ToString();
    }
  }

  return Viet::ReadFileIntoString(GetPluginPath(pluginName, overrideFolder));
}

JsValue DevApi::WritePlugin(const JsFunctionArguments& args)
{
  // TODO: Support multifile plugins?
  auto pluginName = args[1].ToString();
  auto newSources = args[2].ToString();

  std::optional<std::string> overrideFolder;
  if (args.GetSize() >= 4) {
    auto t = args[3].GetType();
    if (t != JsValue::Type::Undefined && t != JsValue::Type::Null) {
      overrideFolder = args[3].ToString();
    }
  }

  auto path = GetPluginPath(pluginName, overrideFolder);

  std::error_code err;
  CreateDirectoryRecursive(path.parent_path().string(), err);
  if (err) {
    throw std::runtime_error("Failed to create directory " +
                             path.parent_path().string() + ": " +
                             err.message());
  }

  std::ofstream f(path);
  f << newSources;
  f.close();
  if (!f) {
    throw std::runtime_error("Failed to write into " + path.string());
  }
  return JsValue::Undefined();
}

JsValue DevApi::GetPlatformVersion(const JsFunctionArguments& args)
{
  return "2.9.0";
}

JsValue DevApi::GetJsMemoryUsage(const JsFunctionArguments& args)
{
  if (!jsEngine) {
    throw NullPointerException("jsEngine");
  }
  return static_cast<double>(jsEngine->GetMemoryUsage());
}

JsValue DevApi::BlockPapyrusEvents(const JsFunctionArguments& args)
{
  bool block = static_cast<bool>(args[1]);
  TESModPlatform::BlockPapyrusEvents(nullptr, -1, nullptr, block);
  return JsValue::Undefined();
}

namespace {
class WrapperScreenShotEventHandler : public RE::MenuEventHandler
{
public:
  WrapperScreenShotEventHandler(RE::MenuEventHandler* originalHandler_)
    : originalHandler(originalHandler_)
  {
  }

  bool CanProcess(RE::InputEvent* e) override
  {
    if (e->eventType == RE::INPUT_EVENT_TYPE::kButton) {
      if (strcmp(e->QUserEvent().c_str(), "Screenshot") == 0) {
        return originalHandler->CanProcess(e);
      }
    }

    return false;
  }
  bool ProcessKinect(RE::KinectEvent* e) override { return false; }
  bool ProcessThumbstick(RE::ThumbstickEvent* e) override { return false; }
  bool ProcessMouseMove(RE::MouseMoveEvent* e) override { return false; }
  bool ProcessButton(RE::ButtonEvent* e) override
  {
    if (strcmp(e->QUserEvent().c_str(), "Screenshot") == 0) {
      return originalHandler->ProcessButton(e);
    }

    return false;
  }

  RE::MenuEventHandler* originalHandler;
};
}

void DevApi::DisableCtrlPrtScnHotkey()
{
  auto mc = RE::MenuControls::GetSingleton();

  RE::MenuEventHandler* originalHandler =
    (RE::MenuEventHandler*)mc->screenshotHandler.get();
  RE::MenuEventHandler* handler =
    (RE::MenuEventHandler*)new WrapperScreenShotEventHandler(originalHandler);

  mc->RemoveHandler(originalHandler);
  mc->AddHandler(handler);
}
