#include "ScampServerListener.h"
#include "ScampServer.h"

ScampServerListener::ScampServerListener(ScampServer &scampServer_)
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
    emit.Call(server.emitter.Value(),
              { Napi::String::New(env, "disconnect"),
                Napi::Number::New(env, userId) });
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

  bool ScampServerListener::OnMpApiEvent(const char* eventName,
                    std::optional<simdjson::dom::element> args,
                    std::optional<uint32_t> formId)
  {
    if (args && !args->is_array()) {
      return true;
    }

    auto mpValue = server.tickEnv.Global().Get("mp");
    if (!mpValue.IsObject()) {
        return true;
    }

    auto mp = mpValue.As<Napi::Object>();

    auto fValue = mp.Get(eventName);
    if (!fValue.IsFunction()) {
      return true;
    }
    auto f = fValue.As<Napi::Function>();

    std::vector<Napi::Value> argumentsInNapiFormat;
    if (formId != std::nullopt) {
      argumentsInNapiFormat.push_back(Napi::Number::New(server.tickEnv, *formId));
    }

    if (args) {
      auto argsArray = args->get_array();
      for (size_t i = 0; i < argsArray.value().size(); ++i) {
        std::string elementString = simdjson::minify(argsArray.value().at(i));

        auto builtinJson = server.tickEnv.Global().Get("JSON").As<Napi::Object>();
        auto builtinParse = builtinJson.Get("parse").As<Napi::Function>();
        auto resultOfParsing = builtinParse.Call(builtinJson, { Napi::String::New(server.tickEnv, elementString) });
        argumentsInNapiFormat.push_back(resultOfParsing);
      }
    }

    try {
      auto callResult = f.Call(argumentsInNapiFormat);

      if (callResult.IsUndefined()) {
        return true;
      }

      // TODO: Handle non-boolean values? Current implementation will throw
      return static_cast<bool>(callResult);
    } catch (std::exception& e) {
      std::cout << "[" << eventName << "] "
                << " " << e.what() << std::endl;
      return true;
    }

    return true;
  }
  
