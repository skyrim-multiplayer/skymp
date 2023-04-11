#include "CustomPropertyBinding.h"

namespace {
  void EnsurePropertyExists(const GamemodeApi::State& state,
                          const std::string& propertyName)
{
  if (!state.createdProperties.count(propertyName)) {
    std::stringstream ss;
    ss << "Property '" << propertyName << "' doesn't exist";
    throw std::runtime_error(ss.str());
  }
}
}

CustomPropertyBinding::CustomPropertyBinding(const std::string &propertyName_) {
  this->propertyName = propertyName_;
}

std::string CustomPropertyBinding::GetPropertyName() const {
  return propertyName;
}

Napi::Value CustomPropertyBinding::Get(Napi::Env env, ScampServer &scampServer, uint32_t formId) {
  auto &partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  EnsurePropertyExists(scampServer.GetGamemodeApiState(), propertyName);
  return refr.GetDynamicFields().Get(env, propertyName);
}

void CustomPropertyBinding::Set(Napi::Env env, ScampServer &scampServer, uint32_t formId, Napi::Value newValue) {
  auto &partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  EnsurePropertyExists(scampServer.GetGamemodeApiState(), propertyName);
  auto& info = scampServer.GetGamemodeApiState().createdProperties.find(propertyName);

  auto newValueDump = NapiHelper::Stringify(env, newValue);
  auto newValueJson = nlohmann::json::parse(newValueDump);

  refr.SetProperty(propertyName, newValueJson, info->second.isVisibleByOwner, info->second.isVisibleByNeighbors);
}