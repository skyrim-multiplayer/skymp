#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "HttpClient.h"
#include "HttpClientApi.h"

TEST_CASE("Asynchronous operations should not trigger assert during static "
          "deinitialization",
          "[JsEngine]")
{
  TaskQueue taskQueue;
  JsEngine engine;
  engine.ResetContext(taskQueue);

  std::stringstream ss;
  ss << "let resolve = null;";
  ss << "let p = new Promise((f) => resolve = f);";
  ss << "p.then(() => {});";
  ss << "resolve()";

  engine.RunScript(ss.str(), "");
}

nlohmann::json ExecuteScript(const char* src)
{
  TaskQueue taskQueue;
  JsEngine engine;
  engine.ResetContext(taskQueue);

  auto global = JsValue::GlobalObject();
  HttpClientApi::Register(global);

  nlohmann::json result;

  JsValue::GlobalObject().SetProperty(
    "resolve", JsValue::Function([&](const JsFunctionArguments& args) {
      result = nlohmann::json::parse(static_cast<std::string>(args[1]));
      return JsValue::Undefined();
    }));

  engine.RunScript(src, "");

  auto startMoment = std::chrono::system_clock::now();
  auto timeout = std::chrono::seconds(60);

  while (1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    taskQueue.Update();
    HttpClientApi::GetHttpClient().Update();

    if (!result.is_null()) {
      break;
    }
    if (std::chrono::system_clock::now() - startMoment > timeout) {
      throw std::runtime_error("Timeout");
    }
  }
  return result;
}

TEST_CASE("Should be able to fetch a resource via https", "[HttpClientApi]")
{
  auto src = R"(
    const client = new HttpClient("https://api.github.com");
    client.get("/yo").then((res) => resolve(JSON.stringify(res)));
  )";

  auto result = ExecuteScript(src);

  nlohmann::json body = nlohmann::json::object();
  body["message"] = "Not Found";
  body["documentation_url"] = "https://docs.github.com/rest";

  REQUIRE(nlohmann::json::parse(result["body"].get<std::string>()) == body);
  REQUIRE(result["status"] == 404);
}

TEST_CASE("Should be able to perform Bearer authorization", "[HttpClientApi]")
{
  auto src = R"(
    const client = new HttpClient("http://httpbin.org");
    const headers = { Authorization: "Bearer 123" };
    client.get("/bearer", headers).then((res) => resolve(JSON.stringify(res)));
  )";

  auto result = ExecuteScript(src);
  REQUIRE(result["status"] == 200);
}

TEST_CASE("Should be able to perform a POST request", "[HttpClientApi]")
{
  auto src = R"(
    const client = new HttpClient("https://httpbin.org");
    const promise = client.post("/post", "{}", "application/json");
    promise.then((res) => resolve(JSON.stringify(res)));
  )";

  auto result = ExecuteScript(src);
  auto responseBody = nlohmann::json::parse(result["body"].get<std::string>());
  REQUIRE(responseBody["url"] == "https://httpbin.org/post");
  REQUIRE(result["status"] == 200);
}
