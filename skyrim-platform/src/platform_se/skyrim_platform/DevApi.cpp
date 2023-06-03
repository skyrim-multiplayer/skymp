#include "DevApi.h"
#include "FileUtils.h"
#include "InvalidArgumentException.h"
#include "NullPointerException.h"
#include "Validators.h"

std::shared_ptr<JsEngine> DevApi::jsEngine = nullptr;
DevApi::NativeExportsMap DevApi::nativeExportsMap;

JsValue DevApi::Require(const JsFunctionArguments& args,
                        const std::vector<const char*>& pluginLoadDirectories)
{
  auto fileName = args[1].ToString();

  if (!ValidateRelativePath(fileName)) {
    throw InvalidArgumentException("fileName", fileName);
  }

  for (auto dir : pluginLoadDirectories) {
    std::filesystem::path filePath =
      std::filesystem::path(dir) / (fileName + ".js");

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
std::filesystem::path GetPluginPath(const std::string& pluginName)
{
  if (!ValidateFilename(pluginName, /*allowDots*/ false)) {
    throw InvalidArgumentException("pluginName", pluginName);
  }
  return std::filesystem::path("Data/Platform/Plugins") / (pluginName + ".js");
}
}

JsValue DevApi::GetPluginSourceCode(const JsFunctionArguments& args)
{
  // TODO: Support multifile plugins?
  auto pluginName = args[1].ToString();
  return Viet::ReadFileIntoString(GetPluginPath(pluginName));
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
  return "2.7.1";
}

JsValue DevApi::GetJsMemoryUsage(const JsFunctionArguments& args)
{
  if (!jsEngine) {
    throw NullPointerException("jsEngine");
  }
  return static_cast<double>(jsEngine->GetMemoryUsage());
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
