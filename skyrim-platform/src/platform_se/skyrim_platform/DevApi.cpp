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

Napi::Value DevApi::Require(
  const Napi::CallbackInfo& info,
  const std::vector<std::filesystem::path>& pluginLoadDirectories)
{
  auto fileName = NapiHelper::ExtractString(info[0], "fileName");

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
    auto runScriptResult = jsEngine->RunScript(src.str(), fileName);
    auto exports =
      NapiHelper::ExtractObject(runScriptResult, "runScriptResult");

    if (auto& f = DevApi::nativeExportsMap[fileName]) {
      exports = f(exports);
    }

    return exports;
  }

  throw std::runtime_error("'" + fileName + "' doesn't exist");
}

Napi::Value DevApi::AddNativeExports(const Napi::CallbackInfo& info)
{
  auto fileName = NapiHelper::ExtractString(info[0], "fileName");
  auto exports = NapiHelper::ExtractObject(info[1], "exports");

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

Napi::Value DevApi::GetPluginSourceCode(const Napi::CallbackInfo& info)
{
  // Multifile plugins unsupported
  auto pluginName = NapiHelper::ExtractString(info[0], "fileName");

  std::optional<std::string> overrideFolder;
  if (info.Length() >= 2) {
    auto overrideFolderCandidate = info[1];
    if (!overrideFolderCandidate.IsNull() &&
        !overrideFolderCandidate.IsUndefined()) {
      overrideFolder =
        NapiHelper::ExtractString(overrideFolderCandidate, "overrideFolder");
    }
  }

  return Napi::String::New(
    info.Env(),
    Viet::ReadFileIntoString(GetPluginPath(pluginName, overrideFolder)));
}

Napi::Value DevApi::WritePlugin(const Napi::CallbackInfo& info)
{
  // Multifile plugins unsupported
  auto pluginName = NapiHelper::ExtractString(info[0], "pluginName");
  auto newSources = NapiHelper::ExtractString(info[1], "newSources");

  std::optional<std::string> overrideFolder;
  if (info.Length() >= 3) {
    auto overrideFolderCandidate = info[2];
    if (!overrideFolderCandidate.IsNull() &&
        !overrideFolderCandidate.IsUndefined()) {
      overrideFolder =
        NapiHelper::ExtractString(overrideFolderCandidate, "overrideFolder");
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
  return info.Env().Undefined();
}

Napi::Value DevApi::GetPlatformVersion(const Napi::CallbackInfo& info)
{
  constexpr auto kSkyrimPlatformVersion = "2.9.0";
  return Napi::String::New(info.Env(), kSkyrimPlatformVersion);
}

Napi::Value DevApi::GetJsMemoryUsage(const Napi::CallbackInfo& info)
{
  if (!jsEngine) {
    throw NullPointerException("jsEngine");
  }
  return Napi::Number::New(info.Env(), jsEngine->GetMemoryUsage());
}

Napi::Value DevApi::BlockPapyrusEvents(const Napi::CallbackInfo& info)
{
  bool block = NapiHelper::ExtractBoolean(info[0], "block");
  TESModPlatform::BlockPapyrusEvents(nullptr, -1, nullptr, block);
  return info.Env().Undefined();
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
