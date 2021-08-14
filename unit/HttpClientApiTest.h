#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "HttpClient.h"
#include "HttpClientApi.h"
#include "JsPromise.h"

TaskQueue taskQueue;
JsEngine engine;

TEST_CASE("Should be able to fetch server list (https get)", "[HttpClientApi]")
{
  engine.ResetContext(taskQueue);

  auto exports = JsValue::Object();

  HttpClientApi::Register(exports);

  auto prototype = exports.GetProperty("HttpClient");
  auto instance = prototype.Constructor({ "https://skymp.io", 80 });

  JsValue result = JsValue::Undefined();
  JsValue error = JsValue::Undefined();

  JsPromise promise =
    instance.GetProperty("get").Call({ instance, "/api/servers" });
  promise.Then([&result](const JsFunctionArguments& args) {
    result = args[1];
    return JsValue::Undefined();
  });
  promise.Catch([&error](const JsFunctionArguments& args) {
    error = args[1];
    return JsValue::Undefined();
  });

  auto startMoment = std::chrono::system_clock::now();
  auto timeout = std::chrono::seconds(5);

  while (1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // taskQueue.Update();
    HttpClientApi::GetHttpClient().Update();

    if (result.GetType() != JsValue::Type::Undefined) {
      throw std::runtime_error("Success: " + result.ToString());
    }
    if (error.GetType() != JsValue::Type::Undefined) {
      throw std::runtime_error("Bad things happen: " + error.ToString());
    }
    if (std::chrono::system_clock::now() - startMoment > timeout) {
      throw std::runtime_error("Timeout");
    }
  }
}