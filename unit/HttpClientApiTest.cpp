#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "HttpClient.h"
#include "HttpClientApi.h"

TEST_CASE("Asynchronous operations should not trigger assert during static "
          "deinitialization",
          "[HttpClientApi][JsEngine]")
{
  Viet::TaskQueue taskQueue;
  JsEngine engine;
  engine.ResetContext(taskQueue);

  // Triggers OnPromiseContinuation that adds a task to the queue.
  auto src = R"(
    let resolve = null;
    let p = new Promise((f) => resolve = f);
    p.then(() => {});
    resolve()
  )";
  engine.RunScript(src, "");

  // We never call TaskQueue::Update, so task never runs.
  // TaskQueue destroys after JsEngine, tries to destroy the tasks.
  // Tasks destroy captured things (JsValue in the old implementation).
  // In the old implementation, JsValue dtor triggers an assert.
}

nlohmann::json ExecuteScript(const char* src)
{
  Viet::TaskQueue taskQueue;
  JsEngine engine;
  engine.ResetContext(taskQueue);

  HttpClientApi::Register(JsValue::GlobalObject());

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
    HttpClientApi::GetHttpClient().ExecuteQueuedCallbacks();

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
    const options = { headers };
    client.get("/bearer", options).then((res) => resolve(JSON.stringify(res)));
  )";

  auto result = ExecuteScript(src);
  auto responseBody = nlohmann::json::parse(result["body"].get<std::string>());
  REQUIRE(responseBody["authenticated"] == true);
  REQUIRE(responseBody["token"] == "123");
  REQUIRE(result["status"] == 200);
}

TEST_CASE("Should be able to perform a POST request", "[HttpClientApi]")
{
  auto src = R"(
    const client = new HttpClient("https://httpbin.org");
    const options = { body: "{}", contentType: "application/json" };
    const promise = client.post("/post", options);
    promise.then((res) => resolve(JSON.stringify(res)));
  )";

  auto result = ExecuteScript(src);
  auto responseBody = nlohmann::json::parse(result["body"].get<std::string>());
  REQUIRE(responseBody["url"] == "https://httpbin.org/post");
  REQUIRE(result["status"] == 200);
}
