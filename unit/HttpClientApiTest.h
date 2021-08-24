#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "HttpClient.h"
#include "HttpClientApi.h"

TEST_CASE("Should be able to fetch a resource via https", "[HttpClientApi]")
{
  /*TaskQueue taskQueue;
  JsEngine engine;
  engine.ResetContext(taskQueue);

  std::stringstream ss;
  ss << "let resolve = null;";
  ss << "let p = new Promise((f) => resolve = f);";
  ss << "p.then(() => {});";
  ss << "resolve()";

  engine.RunScript(ss.str(), "");*/

  TaskQueue taskQueue;
  JsEngine engine;
  engine.ResetContext(taskQueue);
  HttpClientApi::Register(JsValue::GlobalObject());

  nlohmann::json result;

  JsValue::GlobalObject().SetProperty(
    "resolve", JsValue::Function([&](const JsFunctionArguments& args) {
      result = nlohmann::json::parse(static_cast<std::string>(args[1]));
      return JsValue::Null();
    }));

  auto src = R"(
    const client = new HttpClient("https://api.github.com", 80);
    client.get("/yo").then((res) => resolve(JSON.stringify(res)));
  )";

  engine.RunScript(src, "");

  auto startMoment = std::chrono::system_clock::now();
  auto timeout = std::chrono::seconds(5000);

  while (1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    taskQueue.Update();
    HttpClientApi::GetHttpClient().Update();

    if (result != nullptr) {
      break;
    }
    if (std::chrono::system_clock::now() - startMoment > timeout) {
      throw std::runtime_error("Timeout");
    }
  }

  nlohmann::json body = nlohmann::json::object();
  body["message"] = "Not Found";
  body["documentation_url"] = "https://docs.github.com/rest";

  REQUIRE(nlohmann::json::parse(result["body"].get<std::string>()) == body);
  REQUIRE(result["status"] == 404);
}
