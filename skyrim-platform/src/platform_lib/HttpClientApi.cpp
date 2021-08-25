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

inline HttpClient::Headers JsHeadersToCppHeaders(const JsValue& headers)
{
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
  JsValue path = args[1], headers = args[2],
          host = args[0].GetProperty("host");

  JsValue resolverFn = JsValue::Function([=](const JsFunctionArguments& args) {
    auto resolve = std::make_shared<JsValue>(args[1]);
    auto pathStr = static_cast<std::string>(path);
    auto hostStr = static_cast<std::string>(host);
    g_httpClient.Get(
      hostStr.data(), pathStr.data(), JsHeadersToCppHeaders(headers),
      [=](const HttpClient::HttpResult& res) {
        auto result = JsValue::Object();
        result.SetProperty(
          "body",
          JsValue::String(std::string{ res.body.begin(), res.body.end() }));
        result.SetProperty("status", res.status);
        resolve->Call({ JsValue::Undefined(), result });
      });

    return JsValue::Undefined();
  });

  auto promise = CreatePromise(resolverFn);

  return promise;
}

JsValue HttpClientApi::Post(const JsFunctionArguments& args)
{
  JsValue path = args[1], body = args[2], contentType = args[3],
          headers = args[4], host = args[0].GetProperty("host");

  auto resolverFn = JsValue::Function([=](const JsFunctionArguments& args) {
    auto resolve = std::make_shared<JsValue>(args[1]);
    auto pathStr = static_cast<std::string>(path);
    auto bodyStr = static_cast<std::string>(body);
    auto hostStr = static_cast<std::string>(host);
    auto contentTypeStr = static_cast<std::string>(contentType);
    g_httpClient.Post(
      hostStr.data(), pathStr.data(), bodyStr.data(), contentTypeStr.data(),
      JsHeadersToCppHeaders(headers),
      [=](const HttpClient::HttpResult& res) {
        auto result = JsValue::Object();
        result.SetProperty(
          "body",
          JsValue::String(std::string{ res.body.begin(), res.body.end() }));
        result.SetProperty("status", res.status);
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
