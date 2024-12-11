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

bool StartsWith(const std::string& str, const char* prefix)
{
  return str.compare(0, strlen(prefix), prefix) == 0;
}

}

CustomPropertyBinding::CustomPropertyBinding(const std::string& propertyName_)
{
  this->propertyName = propertyName_;
  this->isPrivate =
    StartsWith(propertyName_, MpObjectReference::GetPropertyPrefixPrivate());
  this->isPrivateIndexed = StartsWith(
    propertyName_, MpObjectReference::GetPropertyPrefixPrivateIndexed());
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

  if (!isPrivate) {
    EnsurePropertyExists(scampServer.GetGamemodeApiState(), propertyName);
  }

  auto& fynamicFields = refr.GetDynamicFields();
  auto& entry = dynamicFields.Get(propertyName);

  if (entry.cache != std::nullopt) {
    switch (entry.cache->index()) {
      case static_cast<size_t>(DynamicFieldsEntryCacheIndex::kVoidPtr):

// TODO: !!!!!!!!!!!!!!!!!! Bad type in std::get<DynamicFieldsValueObject>(*entry.cache)

        if (auto ptr = std::get<void*>(*entry.cache)) {
          auto napiValue = static_cast<napi_value>(ptr);
          auto napiValueWrapped = Napi::Value(env, napiValue);
          return structuredClone.Value().Call(
            { env.Null(), napiValueWrapped });
        } else {
          return env.Null();
        }
      case static_cast<size_t>(DynamicFieldsEntryCacheIndex::kDouble):
        return Napi::Number::New(env, std::get<double>(*entry.cache));
      case static_cast<size_t>(DynamicFieldsEntryCacheIndex::kBool):
        return Napi::Boolean::New(env, std::get<bool>(*entry.cache));
      case static_cast<size_t>(DynamicFieldsEntryCacheIndex::kString):
        return Napi::String::New(env, std::get<std::string>(*entry.cache));
      default:
        spdlog::error(
          "CustomPropertyBinding::Get {:x} {} - unknown cache type {}", formId,
          propertyName, entry.cache->index());
    }
  }

  return NapiHelper::ParseJson(env, entry.value);
}

void CustomPropertyBinding::Set(Napi::Env env, ScampServer& scampServer,
                                uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  auto& state = scampServer.GetGamemodeApiState();

  auto newValueDump = NapiHelper::Stringify(env, newValue);
  auto newValueJson = nlohmann::json::parse(newValueDump);

  auto& fynamicFields = refr.GetDynamicFields();
  auto& entry = dynamicFields.Get(propertyName);

  // If there was an object in the cache, we need to free it before setting a
  // new value, otherwise we'll have a memory leak
  if (entry.cache != std::nullopt) {
    switch (entry.cache->index()) {
      case static_cast<size_t>(DynamicFieldsEntryCacheIndex::kVoidPtr):

        // TODO: napi_delete_reference instead!!!!!!!!!!!!

        if (auto ptr = std::get<DynamicFieldsValueObject>(*entry.cache)) {
          uint32_t unrefResult;
          auto res = napi_reference_unref(env, static_cast<napi_ref>(ptr),
                                          &unrefResult);
          if (res != napi_ok) {
            spdlog::error("CustomPropertyBinding::Set {:x} {} - failed to "
                          "unref napi reference, res={}",
                          formId, propertyName, res);
          } else {
            spdlog::info("unref result: {}", unrefResult);
          }
        }
        break;
      default:
        break;
    }
  }

  std::optional<DynamicFieldsEntryCache> newValueCache;

  switch (newValue.Type()) {
    case napi_undefined:
      newValueCache = static_cast<void*>(nullptr);
      break;
    case napi_null:
      newValueCache = static_cast<void*>(nullptr);
      break;
    case napi_boolean:
      newValueCache = newValue.As<Napi::Boolean>().Value();
      break;
    case napi_number:
      newValueCache = newValue.As<Napi::Number>().DoubleValue();
      break;
    case napi_string:
      newValueCache = newValue.As<Napi::String>().Utf8Value();
      break;
    case napi_symbol:
      newValueJson = nlohmann::json{};
      newValueDump = "null";
      newValueCache = static_cast<void*>(nullptr);
      spdlog::error(
        "CustomPropertyBinding::Set {:x} {} - symbol type is not supported",
        formId, propertyName);
      break;
    case napi_object: {
      auto obj = newValue.As<Napi::Object>();

      auto napiValue = static_cast<napi_value>(newValue);
      napi_ref ref;
      auto res = napi_create_reference(env, napiValue, 1, &ref);
      if (res != napi_ok) {
        spdlog::error("CustomPropertyBinding::Set {:x} {} - failed to "
                      "create napi reference, res={}",
                      formId, propertyName, res);
        newValueJson = nlohmann::json{};
        newValueDump = "null";
        newValueCache = static_cast<void*>(nullptr);
      } else {
        newValueJson = nlohmann::json{};
        newValueDump = "null";
        newValueCache = static_cast<void*>(ref);
      }

    } break;
    case napi_function:
      newValueJson = nlohmann::json{};
      newValueDump = "null";
      newValueCache = static_cast<void*>(nullptr);
      spdlog::error("CustomPropertyBinding::Set {:x} {} - function type "
                    "is not supported",
                    formId, propertyName);
      break;
    case napi_external:
      newValueJson = nlohmann::json{};
      newValueDump = "null";
      newValueCache = static_cast<void*>(nullptr);
      spdlog::error("CustomPropertyBinding::Set {:x} {} - external type "
                    "is not supported",
                    formId, propertyName);
      break;
    case napi_bigint:
      newValueJson = nlohmann::json{};
      newValueDump = "null";
      newValueCache = static_cast<void*>(nullptr);
      spdlog::error("CustomPropertyBinding::Set {:x} {} - bigint type is "
                    "not supported",
                    formId, propertyName);
      break;
  }

  if (isPrivate) {
    refr.SetProperty(propertyName, newValueJson, newValueCache, false, false);
    if (isPrivateIndexed) {
      refr.RegisterPrivateIndexedProperty(propertyName, newValueDump);
    }
    return;
  }
  auto it = EnsurePropertyExists(state, propertyName);
  refr.SetProperty(propertyName, newValueJson, newValueCache,
                   it->second.isVisibleByOwner,
                   it->second.isVisibleByNeighbors);
}
