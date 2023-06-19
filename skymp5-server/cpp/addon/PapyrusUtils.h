#pragma once
#include "EspmGameObject.h"
#include "FormDesc.h"
#include "MpFormGameObject.h"
#include "NapiHelper.h"
#include "WorldState.h"
#include "papyrus-vm/Structures.h"
#include <napi.h>
#include <sstream>

class PapyrusUtils
{
public:
  static Napi::Value GetJsObjectFromPapyrusObject(
    Napi::Env env, const VarValue& value,
    const std::vector<std::string>& espmFilenames)
  {
    auto ptr = static_cast<IGameObject*>(value);
    if (!ptr) {
      return env.Null();
    }

    if (auto concrete = dynamic_cast<EspmGameObject*>(ptr)) {
      auto rawId = concrete->record.rec->GetId();
      auto id = concrete->record.ToGlobalId(rawId);

      auto desc = FormDesc::FromFormId(id, espmFilenames).ToString();

      auto result = Napi::Object::New(env);
      result.Set("type", Napi::String::New(env, "espm"));
      result.Set("desc", Napi::String::New(env, desc));
      return result;
    }

    if (auto concrete = dynamic_cast<MpFormGameObject*>(ptr)) {
      auto formId =
        concrete->GetFormPtr() ? concrete->GetFormPtr()->GetFormId() : 0;

      auto desc = FormDesc::FromFormId(formId, espmFilenames).ToString();

      auto result = Napi::Object::New(env);
      result.Set("type", Napi::String::New(env, "form"));
      result.Set("desc", Napi::String::New(env, desc));
      return result;
    }

    throw std::runtime_error(
      "This type of IGameObject is not supported in JS");
  }

  static Napi::Value GetJsValueFromPapyrusValue(
    Napi::Env env, const VarValue& value,
    const std::vector<std::string>& espmFilenames)
  {
    if (value.promise) {
      Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

      value.promise->Then([deferred, espmFilenames](const VarValue& v) {
        auto value =
          GetJsValueFromPapyrusValue(deferred.Env(), v, espmFilenames);
        deferred.Resolve(value);
      });

      value.promise->Catch([deferred, espmFilenames](const char* what) {
        auto error = Napi::String::New(deferred.Env(), what);
        deferred.Reject(error);
      });
      return deferred.Promise();
    }
    switch (value.GetType()) {
      case VarValue::kType_Object:
        return GetJsObjectFromPapyrusObject(env, value, espmFilenames);
      case VarValue::kType_Identifier:
        throw std::runtime_error(
          "Unexpected convertion from Papyrus identifier");
      case VarValue::kType_String: {
        std::string str = static_cast<const char*>(value);
        return Napi::String::New(env, str);
      }
      case VarValue::kType_Integer: {
        auto v = static_cast<int32_t>(value);
        return Napi::Number::New(env, v);
      }
      case VarValue::kType_Float: {
        auto v = static_cast<double>(value);
        return Napi::Number::New(env, v);
      }
      case VarValue::kType_Bool: {
        auto v = static_cast<bool>(value);
        return Napi::Boolean::New(env, v);
      }

      case VarValue::kType_ObjectArray:
      case VarValue::kType_StringArray:
      case VarValue::kType_IntArray:
      case VarValue::kType_FloatArray:
      case VarValue::kType_BoolArray: {
        if (value.pArray == nullptr) {
          return env.Null();
        }
        auto arr = Napi::Array::New(env, value.pArray->size());
        auto n = arr.Length();
        for (uint32_t i = 0; i < n; ++i) {
          arr.Set(i,
                  GetJsValueFromPapyrusValue(env, (*value.pArray)[i],
                                             espmFilenames));
        }
        return arr;
      }
    }
    std::stringstream ss;
    ss << "Could not convert a Papyrus value " << value << " to JS format";
    throw std::runtime_error(ss.str());
  }

  static VarValue GetPapyrusValueFromJsValue(const Napi::Value& v,
                                             bool treatNumberAsInt,
                                             WorldState& wst)
  {
    switch (v.Type()) {
      case napi_undefined:
        return VarValue::None();
      case napi_null:
        return VarValue::None();
      case napi_boolean:
        return VarValue(v.As<Napi::Boolean>().Value());
      case napi_number:
        return treatNumberAsInt ? VarValue(v.As<Napi::Number>().Int32Value())
                                : VarValue(v.As<Napi::Number>().DoubleValue());
      case napi_string:
        return VarValue(v.As<Napi::String>().Utf8Value());
      case napi_symbol:
        throw std::runtime_error("Symbol is not supported");
      case napi_object: {
        auto obj = v.As<Napi::Object>();
        if (obj.IsArray()) {
          auto arr = v.As<Napi::Array>();
          auto n = arr.Length();
          if (n == 0) {
            // Treat zero-length arrays as kType_ObjectArray ("none array")
            VarValue papyrusArray(VarValue::kType_ObjectArray);
            papyrusArray.pArray = std::make_shared<std::vector<VarValue>>();
            return papyrusArray;
          }

          auto arrayContents = std::make_shared<std::vector<VarValue>>();
          uint8_t type = ~0;

          for (uint32_t i = 0; i < n; ++i) {
            arrayContents->push_back(
              GetPapyrusValueFromJsValue(arr.Get(i), treatNumberAsInt, wst));
            auto extractedType = arrayContents->back().GetType();
            if (type == static_cast<uint8_t>(~0)) {
              type = extractedType;
            } else if (extractedType != type) {
              throw std::runtime_error(
                "Papyrus doesn't support heterogeneous arrays");
            }
          }

          VarValue papyrusArray(
            ActivePexInstance::GetArrayTypeByElementType(type));
          papyrusArray.pArray = arrayContents;
          return papyrusArray;
        } else {
          // TODO: consider removing promise support. wouldn't be better if we
          // always wait promises on js side instead of passing to papyrus?
          auto obj = v.As<Napi::Object>();
          if (v.IsPromise()) {
            VarValue res = VarValue::None();
            res.promise = std::make_shared<Viet::Promise<VarValue>>();

            auto thenCallback = Napi::Function::New(
              v.Env(), [res, &wst](const Napi::CallbackInfo& info) {
                // TODO: should we always set treatNumberAsInt to false?
                bool treatNumberAsInt = false;
                res.promise->Resolve(
                  GetPapyrusValueFromJsValue(info[0], treatNumberAsInt, wst));
              });

            auto then = obj.Get("then").As<Napi::Function>();
            then.Call(obj, { thenCallback });

            // TODO: same for obj.Get("catch")?
            return res;
          }

          auto desc = NapiHelper::ExtractString(obj.Get("desc"), "desc");
          auto type = NapiHelper::ExtractString(obj.Get("type"), "type");

          const auto espmFileNames = wst.GetEspm().GetFileNames();
          uint32_t id = FormDesc::FromString(desc).ToFormId(espmFileNames);

          if (type == "form") {
            MpObjectReference& refr = wst.GetFormAt<MpObjectReference>(id);
            return VarValue(std::make_shared<MpFormGameObject>(&refr));
          }

          if (type == "espm") {
            auto lookupRes = wst.GetEspm().GetBrowser().LookupById(id);
            if (!lookupRes.rec) {
              std::stringstream ss;
              ss << "ESPM record with id " << std::hex << id
                 << " doesn't exist";
              throw std::runtime_error(ss.str());
            }
            return VarValue(std::make_shared<EspmGameObject>(lookupRes));
          }

          std::stringstream ss;
          ss << "Unknown object type '" << type
             << "', must be 'form' | 'espm'";
          throw std::runtime_error(ss.str());
        }
      }
      case napi_function:
        throw std::runtime_error("Function is not supported");
      case napi_external:
        throw std::runtime_error("External is not supported");
      case napi_bigint:
        throw std::runtime_error("BigInt is not supported");
      default:
        throw std::runtime_error("Unknown JavaScript type " +
                                 std::to_string(v.Type()));
    }
  }
};
