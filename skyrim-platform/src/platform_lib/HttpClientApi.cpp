#include "HttpClientApi.h"
#include "CreatePromise.h"
#include "HttpClient.h"

namespace {
HttpClient g_httpClient;

template <class F>
inline void IterateKeys(const JsValue& object, F fn)
{
  if (object.GetType() != JsType::Object) {
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
  if (options.GetType() != JsType::Object) {
    return HttpClient::Headers();
  }

  auto headers = options.GetProperty("headers");
  if (headers.GetType() != JsType::Object) {
    return HttpClient::Headers();
  }

  HttpClient::Headers res;
  IterateKeys(headers, [&](const JsValue& key) {
    res.push_back({ static_cast<std::string>(key),
                    static_cast<std::string>(headers.GetProperty(key)) });
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
  JsValue path = args[1], options = args[2],
          host = args[0].GetProperty("host");

  JsValue resolverFn = JsValue::Function([=](const JsFunctionArguments& args) {
    auto resolve = std::make_shared<JsValue>(args[1]);
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
        resolve->Call({ JsValue::Undefined(), result });
      });

    return JsValue::Undefined();
  });

  auto promise = CreatePromise(resolverFn);

  return promise;
}

JsValue HttpClientApi::Post(const JsFunctionArguments& args)
{
  JsValue path = args[1], options = args[2],
          host = args[0].GetProperty("host");

  auto resolverFn = JsValue::Function([=](const JsFunctionArguments& args) {
    auto resolve = std::make_shared<JsValue>(args[1]);
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
        resolve->Call({ JsValue::Undefined(), result });
      });

    return JsValue::Undefined();
  });

  return CreatePromise(resolverFn);
}

HttpClient& HttpClientApi::GetHttpClient()
{
  return g_httpClient;
}
