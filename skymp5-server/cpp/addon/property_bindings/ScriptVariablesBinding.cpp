#include "ScriptVariablesBinding.h"

#include "../PapyrusUtils.h"

namespace {
template <class F>
void ForEachScriptVariable(const std::shared_ptr<IGameObject>& gameObject,
                           const F& callback)
{
  auto& activePexInstances = gameObject->GetActivePexInstances();
  for (auto& activePexInstance : activePexInstances) {
    auto variablesHolder = activePexInstance->GetVariablesHolder();
    auto& scriptName = activePexInstance->GetSourcePexName();
    auto& variables = variablesHolder->ListVariables();
    callback(scriptName, variables);
  }
}
}

Napi::Value ScriptVariablesBinding::Get(Napi::Env env,
                                        ScampServer& scampServer,
                                        uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  auto gameObject = actor.ToGameObject();
  if (!gameObject) {
    spdlog::error("ScriptVariablesBinding - ToGameObject() returned nullptr");
    return Napi::Array::New(env);
  }

  size_t arrayLength = 0;

  ForEachScriptVariable(
    gameObject,
    [&](const std::string& scriptName, const CIMap<VarValue>& variables) {
      arrayLength += variables.size();
    });

  auto array = Napi::Array::New(env, arrayLength);

  size_t counter = 0;
  ForEachScriptVariable(
    gameObject,
    [&](const std::string& scriptName, const CIMap<VarValue>& variables) {
      for (auto pair : variables) {
        auto element = Napi::Object::New(env);
        element.Set("scriptName", Napi::String::New(env, scriptName));
        element.Set("variableName", Napi::String::New(env, pair.first.data()));
        element.Set("value",
                    PapyrusUtils::GetJsValueFromPapyrusValue(
                      env, pair.second, partOne->worldState.espmFiles));
        array[counter] = element;
      }
      ++counter;
    });

  return array;
}
