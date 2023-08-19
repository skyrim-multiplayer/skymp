#pragma once

#include "NiPoint3.h"
#include <napi.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

class NapiHelper
{
public:
  static Napi::Value RunScript(const Napi::Env& env, const std::string& src)
  {
    auto eval = env.Global().Get("eval");
    auto evalFunc = eval.As<Napi::Function>();
    return evalFunc.Call({ Napi::String::New(env, src) });
  }

  static uint32_t ExtractUInt32(const Napi::Value& v, const char* argName)
  {
    if (!v.IsNumber()) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be number, but got '";
      ss << Stringify(v.Env(), v);
      ss << "'";
      throw std::runtime_error(ss.str());
    }
    return v.As<Napi::Number>().Uint32Value();
  }

  static float ExtractFloat(const Napi::Value& v, const char* argName)
  {
    if (!v.IsNumber()) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be number, but got '";
      ss << Stringify(v.Env(), v);
      ss << "'";
      throw std::runtime_error(ss.str());
    }
    return v.As<Napi::Number>().FloatValue();
  }

  static bool ExtractBoolean(const Napi::Value& v, const char* argName)
  {
    if (!v.IsBoolean()) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be boolean, but got '";
      ss << Stringify(v.Env(), v);
      ss << "'";
      throw std::runtime_error(ss.str());
    }
    return static_cast<bool>(v.As<Napi::Boolean>());
  }

  static std::string ExtractString(
    const Napi::Value& v, const char* argName,
    std::optional<std::string> alphabet = std::nullopt,
    std::pair<size_t, size_t>
      minMaxSize = { 0, std::numeric_limits<size_t>::max() },
    std::function<bool(std::string)> isUnique = [](auto) { return true; })
  {
    if (!v.IsString()) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be string, but got '";
      ss << Stringify(v.Env(), v);
      ss << "'";
      throw std::runtime_error(ss.str());
    }

    auto result = static_cast<std::string>(v.As<Napi::String>());

    if (alphabet &&
        result.find_first_not_of(alphabet->data()) != std::string::npos) {
      std::stringstream ss;
      ss << "'" << argName
         << "' may contain only Latin characters, numbers, "
            "and underscore";
      throw std::runtime_error(ss.str());
    }

    if (result.size() < minMaxSize.first ||
        result.size() > minMaxSize.second) {
      std::stringstream ss;
      ss << "The length of '" << argName << "' must be between "
         << minMaxSize.first << " and " << minMaxSize.second
         << ", but "
            "it "
            "is '";
      ss << result.size();
      ss << "'";
      throw std::runtime_error(ss.str());
    }

    if (!isUnique(result)) {
      std::stringstream ss;
      ss << "'" << argName << "' must be unique";
      throw std::runtime_error(ss.str());
    }

    return result;
  }

  static Napi::Function ExtractFunction(const Napi::Value& v,
                                        const char* argName)
  {
    if (!v.IsFunction()) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be function, but got '";
      ss << Stringify(v.Env(), v);
      ss << "'";
      throw std::runtime_error(ss.str());
    }
    return v.As<Napi::Function>();
  }

  static Napi::Object ExtractObject(const Napi::Value& v, const char* argName)
  {
    if (!v.IsObject()) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be object, but got '";
      ss << Stringify(v.Env(), v);
      ss << "'";
      throw std::runtime_error(ss.str());
    }
    return v.As<Napi::Object>();
  }

  static Napi::Uint8Array ExtractUInt8Array(const Napi::Value& v,
                                            const char* argName)
  {
    if (!v.IsTypedArray()) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be typed array, but got '";
      ss << v.ToString();
      ss << "'";
      throw std::runtime_error(ss.str());
    }

    auto typedArray = v.As<Napi::TypedArray>();
    if (typedArray.TypedArrayType() != napi_uint8_array) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be Uint8Array, but got '";
      ss << v.ToString();
      ss << "'";
      throw std::runtime_error(ss.str());
    }

    return typedArray.As<Napi::Uint8Array>();
  }

  static Napi::Array ExtractArray(const Napi::Value& v, const char* argName)
  {
    if (!v.IsArray()) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be array, but got '";
      ss << Stringify(v.Env(), v);
      ss << "'";
      throw std::runtime_error(ss.str());
    }
    return v.As<Napi::Array>();
  }

  static NiPoint3 ExtractNiPoint3(const Napi::Value& v, const char* argName)
  {
    if (!v.IsArray()) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be array, but got '";
      ss << Stringify(v.Env(), v);
      ss << "'";
      throw std::runtime_error(ss.str());
    }

    auto array = v.As<Napi::Array>();
    uint32_t length = array.Length();

    static constexpr uint32_t kExpectedLength = 3;
    if (length != kExpectedLength) {
      std::stringstream ss;
      ss << "Expected '" << argName << "' to be array with length "
         << kExpectedLength << ", but got '";
      ss << Stringify(v.Env(), v);
      ss << "'";
      throw std::runtime_error(ss.str());
    }

    NiPoint3 result;
    for (uint32_t i = 0; i < length; ++i) {
      std::string subArgName =
        std::string(argName) + "." + std::to_string(length);
      result[i] = ExtractFloat(array.Get(i), subArgName.data());
    }
    return result;
  }

  static Napi::Value ParseJson(Napi::Env env, const nlohmann::json& jsonValue)
  {
    return ParseJson(env, jsonValue.dump());
  }

  static Napi::Value ParseJson(Napi::Env env, const std::string& jsonValueDump)
  {
    auto builtinJson = env.Global().Get("JSON").As<Napi::Object>();
    auto builtinParse = builtinJson.Get("parse").As<Napi::Function>();
    auto result = builtinParse.Call(builtinJson,
                                    { Napi::String::New(env, jsonValueDump) });
    return result;
  }

  static std::string Stringify(Napi::Env env, Napi::Value value)
  {
    auto builtinJson = env.Global().Get("JSON").As<Napi::Object>();
    auto builtinStringify = builtinJson.Get("stringify").As<Napi::Function>();
    auto result = builtinStringify.Call(builtinJson, { value });
    return static_cast<std::string>(result.As<Napi::String>());
  }
};
