#include "CustomPropertyBinding.h"
#include "NapiHelper.h"

namespace {
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
    return;
  }
  auto it = EnsurePropertyExists(state, propertyName);
  refr.SetProperty(propertyName, newValueJson, it->second.isVisibleByOwner,
                   it->second.isVisibleByNeighbors);
}
