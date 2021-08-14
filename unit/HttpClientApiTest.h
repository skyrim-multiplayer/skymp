#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "HttpClient.h"
#include "HttpClientApi.h"

auto src = R"(
  const client = new HttpClient("https://skymp.io", 80);
  client.get("/api/plz404").then((res) => resolve(JSON.stringify(res)));
)";

TEST_CASE("Should be able to fetch a resource via https", "[HttpClientApi]")
{
  TaskQueue taskQueue;
  JsEngine engine;
  engine.ResetContext(taskQueue);
  HttpClientApi::Register(JsValue::GlobalObject());

  auto result = JsValue::Undefined();

  JsValue::GlobalObject().SetProperty(
    "resolve", JsValue::Function([&](const JsFunctionArguments& args) {
      result = args[1];
      return JsValue::Undefined();
    }));

  engine.RunScript(src, "");

  auto startMoment = std::chrono::system_clock::now();
  auto timeout = std::chrono::seconds(5);

  while (1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    taskQueue.Update();
    HttpClientApi::GetHttpClient().Update();

    if (result.GetType() != JsValue::Type::Undefined) {
      break;
    }
    if (std::chrono::system_clock::now() - startMoment > timeout) {
      throw std::runtime_error("Timeout");
    }
  }

  auto j = nlohmann::json::parse(static_cast<std::string>(result));

  REQUIRE(j == nlohmann::json{ { "body", "Not Found" }, { "status", 404 } });
}