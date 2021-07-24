#include "SystemPolyfill.h"

#include "NullPointerException.h"
#include <sstream>

/*std::shared_ptr<JsEngine>* SystemPolyfill::engine = nullptr;
SystemPolyfill::RegisterFn SystemPolyfill::registerFn = nullptr;

namespace {

static std::vector<std::string> moduleNameStack;

const auto fileDir =
  std::filesystem::path("C:/projects/skyrim-multiplayer/build/_client");

JsValue RegisterImpl(const JsValue& dependenciesArray, const JsValue& factory)
{
  if (!SystemPolyfill::engine || !*SystemPolyfill::engine)
    throw NullPointerException("SystemPolyfill::engine");

  std::string myModuleName;
  if (moduleNameStack.size() == 0)
    myModuleName = "index";
  else
    myModuleName = moduleNameStack.back();

  auto exports = JsValue::Object();
  auto exportsF =
    JsValue::Function([&](const JsFunctionArguments& args) -> JsValue {
      auto exportName = args[1];
      auto exportValue = args[2];
      exports.SetProperty(exportName, exportValue);
      return JsValue::Undefined();
    });

  auto context = JsValue::Object();
  context.SetProperty("id", myModuleName);

  auto obj = factory.Call({ exportsF, context });

  auto setters = obj.GetProperty("setters");
  auto numSetters = (int)setters.GetProperty("length");
  for (int i = 0; i < numSetters; ++i) {

    auto moduleName = (std::string)dependenciesArray.GetProperty(i);
    if (moduleName.size() >= 2 && moduleName[0] == '.' && moduleName[1] == '/')
      moduleName = { moduleName.begin() + 2, moduleName.end() };

    std::filesystem::path filePath = fileDir /
      std::filesystem::path(myModuleName).parent_path() / (moduleName + ".js");

    // auto filePath = fileDir / (moduleName + ".js");

    if (!std::filesystem::exists(filePath))
      throw std::runtime_error("'" + filePath.string() + "' doesn't exist");

    std::ifstream t(filePath);
    if (!t.is_open())
      throw std::runtime_error("Failed to open '" + filePath.string() +
                               "' for reading");

    std::stringstream src;
    src << t.rdbuf();

    moduleNameStack.push_back(moduleName);
    auto dependencyExports =
      (**SystemPolyfill::engine).RunScript(src.str(), myModuleName);
    moduleNameStack.pop_back();

    auto setter = setters.GetProperty(i);
    setter.Call({ dependencyExports });
  }

  obj.GetProperty("execute").Call({}); // Execute script and fill exports

  SystemPolyfill::registerFn(myModuleName, exports); // + native exports

  return exports;
}
}

JsValue SystemPolyfill::Register_(const JsFunctionArguments& args)
{
  auto dependenciesArray = args[1];
  auto factory = args[2];
  return RegisterImpl(args[1], args[2]);
}*/
