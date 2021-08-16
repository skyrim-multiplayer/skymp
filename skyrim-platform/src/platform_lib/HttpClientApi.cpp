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

  thread_local auto g_builtinKeys =
    JsValue::GlobalObject().GetProperty("Object").GetProperty("keys");
  thread_local auto g_thisArg = JsValue::Undefined();

  auto keys = g_builtinKeys.Call({ g_thisArg, object });
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
  args[0].SetProperty(JsValue::String("port"), args[2]);
  args[0].SetProperty("get", JsValue::Function(Get));
  args[0].SetProperty("post", JsValue::Function(Post));
  return JsValue::Undefined();
}

JsValue HttpClientApi::Get(const JsFunctionArguments& args)
{
  // These are gonna be read inside CreatePromise only, so it is safe
  thread_local JsValue g_path, g_headers, g_host, g_port;
  g_path = args[1];
  g_headers = args[2];
  g_host = args[0].GetProperty("host");
  g_port = args[0].GetProperty("port");

  thread_local auto g_resolverFn =
    JsValue::Function([](const JsFunctionArguments& args) {
      auto resolve = std::make_shared<JsValue>(args[1]);
      auto path = static_cast<std::string>(g_path);
      auto host = static_cast<std::string>(g_host);
      auto port = g_port.GetType() == JsValue::Type::Number
        ? static_cast<int>(g_port)
        : 80;
      g_httpClient.Get(
        host.data(), path.data(), JsHeadersToCppHeaders(g_headers),
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
  return CreatePromise(g_resolverFn);
}

JsValue HttpClientApi::Post(const JsFunctionArguments& args)
{
  // These are gonna be read inside CreatePromise only, so it is safe
  thread_local JsValue g_path, g_body, g_contentType, g_headers, g_host,
    g_port;
  g_path = args[1];
  g_body = args[2];
  g_contentType = args[3];
  g_headers = args[4];
  g_host = args[0].GetProperty("host");
  g_port = args[0].GetProperty("port");

  thread_local auto g_resolverFn =
    JsValue::Function([](const JsFunctionArguments& args) {
      auto resolve = std::make_shared<JsValue>(args[1]);
      auto path = static_cast<std::string>(g_path);
      auto body = static_cast<std::string>(g_body);
      auto host = static_cast<std::string>(g_host);
      auto contentType = static_cast<std::string>(g_contentType);
      auto port = g_port.GetType() == JsValue::Type::Number
        ? static_cast<int>(g_port)
        : 80;
      g_httpClient.Post(
        host.data(), path.data(), body.data(), contentType.data(),
        JsHeadersToCppHeaders(g_headers),
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
  return CreatePromise(g_resolverFn);
}

HttpClient& HttpClientApi::GetHttpClient()
{
  return g_httpClient;
}
