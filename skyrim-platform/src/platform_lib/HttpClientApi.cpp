#include "HttpClientApi.h"
#include "CreatePromise.h"
#include "HttpClient.h"

namespace {
HttpClient g_httpClient;

template <class F>
inline void IterateKeys(const Napi::Value& object, F fn)
{
  auto env = object.Env();

  if (!object.IsObject()) {
    return;
  }

  auto builtinKeys =
    env.Global().Get("Object").As<Napi::Object>().Get("keys");
  auto thisArg = env.Undefined();

  auto keys = builtinKeys.Call(thisArg, { object }).As<Napi::Array>();
  int length = keys.Length();
  for (int i = 0; i < length; ++i) {
    fn(keys.Get(i));
  }
}

inline HttpClient::Headers GetHeaders(const Napi::Value& options)
{
  if (!options.IsObject()) {
    return HttpClient::Headers();
  }

  auto headers = options.As<Napi::Object>().Get("headers");
  if (!headers.IsObject()) {
    return HttpClient::Headers();
  }

  HttpClient::Headers res;
  IterateKeys(headers, [&](const Napi::Value& key) {
    auto value = headers.As<Napi::Object>().Get(key);

    if (!value.IsString()) {
      std::stringstream ss;
      ss << "Expected HTTP header value be a string but got "
         << value.ToString() << ", header key: " << key.ToString();
      throw std::runtime_error(ss.str());
    }

    res.push_back(
      { key.ToString(), value.ToString() });
  });
  return res;
}
}

Napi::Value HttpClientApi::Constructor(const Napi::CallbackInfo &info)
{
  if (!info.This().IsObject()) {
    throw std::runtime_error("thisArg must be an object in HttpClient constructor");
  }

  info.This().As<Napi::Object>().Set(Napi::String::New(info.Env(), "host"), info[0]);
  info.This().As<Napi::Object>().Set("get", Napi::Function::New(info.Env(), NapiHelper::WrapCppExceptions(Get)));
  info.This().As<Napi::Object>().Set("post", Napi::Function::New(info.Env(), NapiHelper::WrapCppExceptions(Post)));
  return info.Env().Undefined();
}

Napi::Value HttpClientApi::Get(const Napi::CallbackInfo &info)
{
  if (!info.This().IsObject()) {
    throw std::runtime_error("thisArg must be an object in HttpClientApi::Get");
  }

  Napi::Value path = info[0], options = info[1], callback = NapiHelper::ExtractFunction(info[2], "callback"),
          host = info.This().As<Napi::Object>().Get("host");

  auto handleGetRequest = [&](Napi::Function resolver) {
    auto pathStr = NapiHelper::ExtractString(path, "path");
    auto hostStr = NapiHelper::ExtractString(host, "host");

    std::shared_ptr<Napi::Reference<Napi::Function>> resolverRef;
    resolverRef.reset(new Napi::Reference<Napi::Function>(Napi::Persistent(resolver)));

    g_httpClient.Get(
      hostStr.data(), pathStr.data(), GetHeaders(options),
      [resolverRef](Napi::Env env, const HttpClient::HttpResult& res) {
        auto result = Napi::Object::New(env);
        result.Set(
          "body",
          Napi::String::New(env, std::string{ res.body.begin(), res.body.end() }));
        result.Set("status", Napi::Number::New(env, res.status));
        result.Set("error", Napi::String::New(env, res.error));
        resolverRef->Value().Call(env.Undefined(), { result });
      });
  };

  if (callback.IsFunction()) {
    handleGetRequest(callback);
    return info.Env().Undefined();
  }

  Napi::Value resolverFn = Napi::Function::New(info.Env(), NapiHelper::WrapCppExceptions([handleGetRequest](const Napi::CallbackInfo &info) {
    auto resolve = NapiHelper::ExtractFunction(info[0], "resolve");
    handleGetRequest(resolve);
    return info.Env().Undefined();
  });

  return CreatePromise(info.Env(), resolverFn);
}

Napi::Value HttpClientApi::Post(const Napi::CallbackInfo &info)
{
  if (!info.This().IsObject()) {
    throw std::runtime_error("thisArg must be an object in HttpClientApi::Post");
  }

  Napi::Value path = info[0], options = Napi::ExtractObject(info[1], "options"), callback = info[2],
          host = info.This().Get("host");

  auto handlePostRequest = [&](Napi::Function resolver) {
    auto pathStr = NapiHelper::ExtractString(path, "path");
    auto hostStr = NapiHelper::ExtractString(host, "host");
    auto bodyStr = NapiHelper::ExtractString(options.Get("body"), "options.body");
    auto type = NapiHelper::ExtractString(options.Get("contentType"), "options.contentType");

    std::shared_ptr<Napi::Reference<Napi::Function>> resolverRef;
    resolverRef.reset(new Napi::Reference<Napi::Function>(Napi::Persistent(resolver)));

    g_httpClient.Post(
      hostStr.data(), pathStr.data(), bodyStr.data(), type.data(),
      GetHeaders(options), [resolverRef](Napi::Env env, const HttpClient::HttpResult& res) {
        auto result = Napi::Object::New(env);
        result.Set(
          "body",
          Napi::String::New(info.Env(), std::string{ res.body.begin(), res.body.end() }));
        result.Set("status", Napi::Number::New(env, res.status));
        result.Set("error", Napi::String::New(res.error));
        resolverRef->Value().Call(info.Env().Undefined(), { result });
      });
  };

  if (callback.IsFunction()) {
    handlePostRequest(callback);
    return info.Env().Undefined();
  }

  auto resolverFn = Napi::Function::New(info.Env(), NapiHelper::WrapCppExceptions([=](const Napi::CallbackInfo &info) {
    auto resolve = NapiHelper::ExtractFunction(info[0], "resolve");
    handlePostRequest(resolve);
    return info.Env().Undefined();
  });

  return CreatePromise(info.Env(), resolverFn);
}

HttpClient& HttpClientApi::GetHttpClient()
{
  return g_httpClient;
}
