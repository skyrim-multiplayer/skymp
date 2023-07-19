#include "CustomPropertyBinding.h"
#include "NapiHelper.h"
#include "PapyrusUtils.h"

namespace {
  std::vector<std::string> SplitByDot(const std::string &s) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, '.')) {
        result.push_back(item);
    }

    return result;
}

auto EnsurePropertyExists(const GamemodeApi::State& state,
                          const std::string& propertyName)
{
  auto it = state.createdProperties.find(propertyName);
  if (it == state.createdProperties.end()) {
    std::stringstream ss;
    ss << "Property '" << propertyName << "' doesn't exist";
    throw std::runtime_error(ss.str());
  }
  return it;
}
}

CustomPropertyBinding::CustomPropertyBinding(const std::string& propertyName_)
{
  this->propertyName = propertyName_;

  static const std::string kPrivatePrefix = "private.";
  this->isPrivate =
    this->propertyName.compare(0, kPrivatePrefix.size(), kPrivatePrefix) == 0;

  // "scriptVariables.<script name>.<variable name>"
  static const std::string kScriptVariablePrefix = "scriptVariables.";
  this->isScriptVariable = 
    this->propertyName.compare(0, kScriptVariablePrefix.size(), kScriptVariablePrefix) == 0;
}

std::string CustomPropertyBinding::GetPropertyName() const
{
  return propertyName;
}

Napi::Value CustomPropertyBinding::Get(Napi::Env env, ScampServer& scampServer,
                                       uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  if (isPrivate) {
    return NapiHelper::ParseJson(env,
                                 refr.GetDynamicFields().Get(propertyName));
  } else if(isScriptVariable) {
    throw std::runtime_error("Not implemented, use 'scriptVariables' property instead");
  } else {
    EnsurePropertyExists(scampServer.GetGamemodeApiState(), propertyName);
    return NapiHelper::ParseJson(env,
                                 refr.GetDynamicFields().Get(propertyName));
  }
}

void CustomPropertyBinding::Set(Napi::Env env, ScampServer& scampServer,
                                uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  auto& state = scampServer.GetGamemodeApiState();

  auto newValueDump = NapiHelper::Stringify(env, newValue);
  auto newValueJson = nlohmann::json::parse(newValueDump);

  if (isPrivate) {
    refr.SetProperty(propertyName, newValueJson, false, false);
  }
  else if (isScriptVariable) {
    auto gameObject = refr.ToGameObject();
    if (!gameObject) {
      return spdlog::error("CustomPropertyBinding - ToGameObject() returned nullptr");
    }

    auto tokens = SplitByDot(propertyName);
    if (tokens.size() < 3) {
      return spdlog::error("CustomPropertyBinding - Bad script variable property name: '{}'", propertyName);
    }

    std::string scriptName = tokens[1];
    std::string variableName = tokens[2];

    auto &instances = gameObject->GetActivePexInstances();
    auto it = std::find_if(instances.begin(), instances.end(), [&](const std::shared_ptr<ActivePexInstance> &instance) {
      return instance->GetSourcePexName() == scriptName;
    });

    if (it == instances.end()) {
      return spdlog::error("CustomPropertyBinding - Object doesn't have such script: '{}'", propertyName);
    }

    auto &activeScript = *it;
    auto variablesHolder = activeScript->GetVariablesHolder();
    if (!variablesHolder) {
      return spdlog::error("CustomPropertyBinding - GetVariablesHolder() returned nullptr");
    }

    if (!variablesHolder->ListVariables().count(CIString{variableName.data()})) {
      return spdlog::error("CustomPropertyBinding - Script doesn't have such variable: '{}'", propertyName);
    }

    activeScript->GetVariableValueByName(nullptr, variableName) = PapyrusUtils::GetPapyrusValueFromJsValue(newValue, false, partOne->worldState);
  }
  else {
    auto it = EnsurePropertyExists(state, propertyName);
    refr.SetProperty(propertyName, newValueJson, it->second.isVisibleByOwner,
                     it->second.isVisibleByNeighbors);
  }
}
