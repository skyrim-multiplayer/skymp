#include "ScampServerListener.h"
#include "PapyrusUtils.h"
#include "ScampServer.h"

ScampServerListener::ScampServerListener(ScampServer& scampServer_)
  : server(scampServer_)
{
}

void ScampServerListener::OnConnect(Networking::UserId userId)
{
  auto& env = server.tickEnv;
  auto emit = server.emit.Value();
  emit.Call(
    server.emitter.Value(),
    { Napi::String::New(env, "connect"), Napi::Number::New(env, userId) });
}

void ScampServerListener::OnDisconnect(Networking::UserId userId)
{
  auto& env = server.tickEnv;
  auto emit = server.emit.Value();
  emit.Call(
    server.emitter.Value(),
    { Napi::String::New(env, "disconnect"), Napi::Number::New(env, userId) });
}

void ScampServerListener::OnCustomPacket(Networking::UserId userId,
                                         const simdjson::dom::element& content)
{
  std::string contentStr = simdjson::minify(content);

  auto& env = server.tickEnv;
  auto emit = server.emit.Value();
  emit.Call(server.emitter.Value(),
            { Napi::String::New(env, "customPacket"),
              Napi::Number::New(env, userId),
              Napi::String::New(env, contentStr) });
}

bool ScampServerListener::OnMpApiEvent(const GameModeEvent& event)
{
  const char* eventName = event.GetName();
  const std::string& eventArgsJsonDump = event.GetArgumentsJsonArray();
  auto [additionalArgs, additionalArgsCount] = event.GetAdditionalArguments();

  simdjson::dom::parser parser;
  auto eventArgsJson = parser.parse(eventArgsJsonDump);

  if (eventArgsJson.error()) {
    spdlog::error(
      "ScampServerListener::OnMpApiEvent {}: failed to parse event "
      "arguments json '{}'",
      event.GetDetailedNameForLogging(), eventArgsJsonDump);
    return true;
  }

  if (!eventArgsJson.value().is_array()) {
    spdlog::error(
      "ScampServerListener::OnMpApiEvent {}: failed to parse event "
      "arguments json '{}'",
      event.GetDetailedNameForLogging(), eventArgsJsonDump);
    return true;
  }

  auto mpValue = server.tickEnv.Global().Get("mp");
  if (!mpValue.IsObject()) {
    spdlog::error("ScampServerListener::OnMpApiEvent {}: failed to get 'mp' "
                  "object from global scope",
                  event.GetDetailedNameForLogging());
    return true;
  }

  auto mp = mpValue.As<Napi::Object>();

  auto fValue = mp.Get(eventName);
  if (!fValue.IsFunction()) {
    // It's ok not to have a handler for an event
    spdlog::trace("ScampServerListener::OnMpApiEvent {}: failed to get '{}' "
                  "function from 'mp' object",
                  event.GetDetailedNameForLogging(), eventName);
    return true;
  }
  auto f = fValue.As<Napi::Function>();

  std::vector<Napi::Value> argumentsInNapiFormat;

  const size_t n = eventArgsJson.value().get_array().size();
  for (size_t i = 0; i < n; ++i) {
    std::string elementString =
      simdjson::minify(eventArgsJson.value().get_array().at(i));

    auto builtinJson = server.tickEnv.Global().Get("JSON").As<Napi::Object>();
    auto builtinParse = builtinJson.Get("parse").As<Napi::Function>();
    auto resultOfParsing = builtinParse.Call(
      builtinJson, { Napi::String::New(server.tickEnv, elementString) });
    argumentsInNapiFormat.push_back(resultOfParsing);
  }

  for (size_t i = 0; i < additionalArgsCount; ++i) {
    argumentsInNapiFormat.push_back(PapyrusUtils::GetJsValueFromPapyrusValue(
      server.tickEnv, additionalArgs[i],
      server.GetPartOne()->worldState.espmFiles));
  }

  try {
    auto callResult = f.Call(argumentsInNapiFormat);

    if (callResult.IsUndefined()) {
      return true;
    }

    // Be careful: callResult.As<Napi::Boolean>() before static_cast<bool> is
    // strictly required
    return static_cast<bool>(callResult.As<Napi::Boolean>());
  } catch (Napi::Error& e) {
    std::string stacktrace;
    try {
      Napi::Value stack = e.Get("stack");
      stacktrace = stack.ToString();
    } catch (std::exception& e) {
      stacktrace =
        fmt::format("<failed to retrieve stack trace: {}>", e.what());
    }
    spdlog::error("'{}' event handler finished with javascript error '{}', "
                  "stack trace:\n{}",
                  eventName, e.what(), stacktrace);
  } catch (std::exception& e) {
    spdlog::error("'{}' event handler finished with c++ exception '{}'",
                  eventName, e.what());
  }

  return true;
}
