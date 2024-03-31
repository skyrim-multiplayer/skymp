#include "HttpClientApi.h"
#include "CreatePromise.h"
#include "HttpClient.h"

namespace {
HttpClient g_httpClient;

template <class F>
inline void IterateKeys(const JsValue& object, F fn)
{
  if (object.GetType() != JsValue::Type::Object) {
    return;
  }

  auto builtinKeys =
    JsValue::GlobalObject().GetProperty("Object").GetProperty("keys");
  auto thisArg = JsValue::Undefined();

  auto keys = builtinKeys.Call({ thisArg, object });
  int length = static_cast<int>(keys.GetProperty("length"));
  for (int i = 0; i < length; ++i) {
    fn(keys.GetProperty(i));
  }
}

inline HttpClient::Headers GetHeaders(const JsValue& options)
{
  if (options.GetType() != JsValue::Type::Object) {
    return HttpClient::Headers();
  }

  auto headers = options.GetProperty("headers");
  if (headers.GetType() != JsValue::Type::Object) {
    return HttpClient::Headers();
  }

  HttpClient::Headers res;
  IterateKeys(headers, [&](const JsValue& key) {
    auto value = headers.GetProperty(key);

    if (value.GetType() != JsValue::Type::String) {
      std::stringstream ss;
      ss << "Expected HTTP header value be a string but got "
         << value.ToString() << ", header key: " << key.ToString();
      throw std::runtime_error(ss.str());
    }

    res.push_back(
      { static_cast<std::string>(key), static_cast<std::string>(value) });
  });
  return res;
}
}

JsValue HttpClientApi::Constructor(const JsFunctionArguments& args)
{
  args[0].SetProperty(JsValue::String("host"), args[1]);
  args[0].SetProperty("get", JsValue::Function(Get));
  args[0].SetProperty("post", JsValue::Function(Post));
  return JsValue::Undefined();
}

JsValue HttpClientApi::Get(const JsFunctionArguments& args)
{
  JsValue path = args[1], options = args[2], callback = args[3],
          host = args[0].GetProperty("host");

  auto handleGetRequest = [&](const std::shared_ptr<JsValue>& resolver) {
    auto pathStr = static_cast<std::string>(path);
    auto hostStr = static_cast<std::string>(host);
    g_httpClient.Get(
      hostStr.data(), pathStr.data(), GetHeaders(options),
      [=](const HttpClient::HttpResult& res) {
        auto result = JsValue::Object();
        result.SetProperty(
          "body",
          JsValue::String(std::string{ res.body.begin(), res.body.end() }));
        result.SetProperty("status", res.status);
        result.SetProperty("error", res.error);
        resolver->Call({ JsValue::Undefined(), result });
      });
  };

  if (callback.GetType() == JsValue::Type::Function) {
    auto resolve = std::make_shared<JsValue>(callback);
    handleGetRequest(resolve);
    return JsValue::Undefined();
  }

  JsValue resolverFn = JsValue::Function([=](const JsFunctionArguments& args) {
    auto resolve = std::make_shared<JsValue>(args[1]);
    handleGetRequest(resolve);
    return JsValue::Undefined();
  });

  return CreatePromise(resolverFn);
}

JsValue HttpClientApi::Post(const JsFunctionArguments& args)
{
  JsValue path = args[1], options = args[2], callback = args[3],
          host = args[0].GetProperty("host");

  auto handlePostRequest = [&](const std::shared_ptr<JsValue>& resolver) {
    auto pathStr = static_cast<std::string>(path);
    auto hostStr = static_cast<std::string>(host);
    auto bodyStr = static_cast<std::string>(options.GetProperty("body"));
    auto type = static_cast<std::string>(options.GetProperty("contentType"));
    g_httpClient.Post(
      hostStr.data(), pathStr.data(), bodyStr.data(), type.data(),
      GetHeaders(options), [=](const HttpClient::HttpResult& res) {
        auto result = JsValue::Object();
        result.SetProperty(
          "body",
          JsValue::String(std::string{ res.body.begin(), res.body.end() }));
        result.SetProperty("status", res.status);
        result.SetProperty("error", res.error);
        resolver->Call({ JsValue::Undefined(), result });
      });
  };

  if (callback.GetType() == JsValue::Type::Function) {
    auto resolve = std::make_shared<JsValue>(callback);
    handlePostRequest(resolve);
    return JsValue::Undefined();
  }

  auto resolverFn = JsValue::Function([=](const JsFunctionArguments& args) {
    auto resolve = std::make_shared<JsValue>(args[1]);
    handlePostRequest(resolve);
    return JsValue::Undefined();
  });

  return CreatePromise(resolverFn);
}

HttpClient& HttpClientApi::GetHttpClient()
{
  return g_httpClient;
}
