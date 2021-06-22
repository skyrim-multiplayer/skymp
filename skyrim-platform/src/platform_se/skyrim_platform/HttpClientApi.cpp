#include "HttpClientApi.h"
#include "CreatePromise.h"
#include "HttpClient.h"
#include <RE\ConsoleLog.h>

extern HttpClient g_httpClient;

JsValue HttpClientApi::Constructor(const JsFunctionArguments& args)
{
  args[0].SetProperty(JsValue::String("host"), args[1]);
  args[0].SetProperty(JsValue::String("port"), args[2]);
  args[0].SetProperty("get", JsValue::Function(Get));
  return JsValue::Undefined();
}

JsValue HttpClientApi::Get(const JsFunctionArguments& args)
{
  thread_local JsValue g_path, g_host, g_port;
  g_path = args[1];
  g_host = args[0].GetProperty("host");
  g_port = args[0].GetProperty("port");

  thread_local auto g_resolverFn =
    JsValue::Function([](const JsFunctionArguments& args) {
      auto resolve = std::shared_ptr<JsValue>(new JsValue(args[1]));
      auto path = (std::string)g_path;
      auto host = (std::string)g_host;
      auto port = g_port.GetType() == JsValue::Type::Number ? (int)g_port : 80;
      g_httpClient.Get(
        host.data(), path.data(), [=](const std::vector<uint8_t>& res) {
          auto result = JsValue::Object();
          result.SetProperty(
            "body", JsValue::String(std::string{ res.begin(), res.end() }));
          resolve->Call({ JsValue::Undefined(), result });
        });

      return JsValue::Undefined();
    });
  return CreatePromise(g_resolverFn);
}