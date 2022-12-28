#include "AsyncSaveStorage.h"
#include "EspmGameObject.h"
#include "FileDatabase.h"
#include "FormCallbacks.h"
#include "GamemodeApi.h"
#include "LocalizationProvider.h"
#include "MigrationDatabase.h"
#include "MongoDatabase.h"
#include "MpFormGameObject.h"
#include "Networking.h"
#include "NetworkingCombined.h"
#include "NetworkingMock.h"
#include "PartOne.h"
#include "ScriptStorage.h"
#include "formulas/TES5DamageFormula.h"
#include <JsEngine.h>
#include <cassert>
#include <cctype>
#include <memory>
#include <napi.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#ifndef NAPI_CPP_EXCEPTIONS
#  error NAPI_CPP_EXCEPTIONS must be defined or throwing from JS code would crash!
#endif

namespace {
inline NiPoint3 NapiValueToNiPoint3(Napi::Value v)
{
  NiPoint3 res;
  auto arr = v.As<Napi::Array>();
  int n = std::min((int)arr.Length(), 3);
  for (int i = 0; i < n; ++i)
    res[i] = arr.Get(i).As<Napi::Number>().FloatValue();
  return res;
}

inline Napi::Value RunScript(const Napi::Env& env, const std::string& src)
{
  auto eval = env.Global().Get("eval");
  auto evalFunc = eval.As<Napi::Function>();
  return evalFunc.Call({ Napi::String::New(env, src) });
}
}

class ScampServerListener;

class Bot
{
public:
  Bot(std::shared_ptr<Networking::IClient> cl_)
    : cl(cl_)
  {
  }

  void Destroy() { cl.reset(); }

  void Send(std::string packet)
  {
    if (cl)
      cl->Send(reinterpret_cast<Networking::PacketData>(packet.data()),
               packet.size(), true);
  }

  void Tick()
  {
    if (cl)
      cl->Tick([](auto, auto, auto, auto, auto) {}, nullptr);
  }

private:
  std::shared_ptr<Networking::IClient> cl;
};

class ScampServer : public Napi::ObjectWrap<ScampServer>
{
  friend class ScampServerListener;

public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  ScampServer(const Napi::CallbackInfo& info);

  Napi::Value AttachSaveStorage(const Napi::CallbackInfo& info);
  Napi::Value Tick(const Napi::CallbackInfo& info);
  Napi::Value On(const Napi::CallbackInfo& info);
  Napi::Value CreateActor(const Napi::CallbackInfo& info);
  Napi::Value SetUserActor(const Napi::CallbackInfo& info);
  Napi::Value GetUserActor(const Napi::CallbackInfo& info);
  Napi::Value GetActorPos(const Napi::CallbackInfo& info);
  Napi::Value GetActorCellOrWorld(const Napi::CallbackInfo& info);
  Napi::Value GetActorName(const Napi::CallbackInfo& info);
  Napi::Value DestroyActor(const Napi::CallbackInfo& info);
  Napi::Value SetRaceMenuOpen(const Napi::CallbackInfo& info);
  Napi::Value GetActorsByProfileId(const Napi::CallbackInfo& info);
  Napi::Value SetEnabled(const Napi::CallbackInfo& info);
  Napi::Value SendCustomPacket(const Napi::CallbackInfo& info);
  Napi::Value CreateBot(const Napi::CallbackInfo& info);
  Napi::Value GetUserByActor(const Napi::CallbackInfo& info);
  Napi::Value ExecuteJavaScriptOnChakra(const Napi::CallbackInfo& info);
  Napi::Value SetSendUiMessageImplementation(const Napi::CallbackInfo& info);
  Napi::Value OnUiEvent(const Napi::CallbackInfo& info);
  Napi::Value Clear(const Napi::CallbackInfo& info);
  Napi::Value WriteLogs(const Napi::CallbackInfo& info);

private:
  void RegisterChakraApi(std::shared_ptr<JsEngine> chakraEngine);

  std::shared_ptr<PartOne> partOne;
  std::shared_ptr<Networking::IServer> server;
  std::shared_ptr<Networking::MockServer> serverMock;
  std::shared_ptr<ScampServerListener> listener;
  Napi::Env tickEnv;
  Napi::ObjectReference emitter;
  Napi::FunctionReference emit;
  Napi::FunctionReference sendUiMessageImplementation;
  std::shared_ptr<spdlog::logger> logger;
  nlohmann::json serverSettings;
  std::shared_ptr<JsEngine> chakraEngine;
  Viet::TaskQueue chakraTaskQueue;
  GamemodeApi::State gamemodeApiState;

  std::shared_ptr<LocalizationProvider> localizationProvider;

  static Napi::FunctionReference constructor;
};

class ScampServerListener : public PartOne::Listener
{
public:
  ScampServerListener(ScampServer& server_)
    : server(server_)
  {
  }

  void OnConnect(Networking::UserId userId) override
  {
    auto& env = server.tickEnv;
    auto emit = server.emit.Value();
    emit.Call(
      server.emitter.Value(),
      { Napi::String::New(env, "connect"), Napi::Number::New(env, userId) });
  }

  void OnDisconnect(Networking::UserId userId) override
  {
    auto& env = server.tickEnv;
    auto emit = server.emit.Value();
    emit.Call(server.emitter.Value(),
              { Napi::String::New(env, "disconnect"),
                Napi::Number::New(env, userId) });
  }

  void OnCustomPacket(Networking::UserId userId,
                      const simdjson::dom::element& content) override
  {
    std::string contentStr = simdjson::minify(content);

    auto& env = server.tickEnv;
    auto emit = server.emit.Value();
    emit.Call(server.emitter.Value(),
              { Napi::String::New(env, "customPacket"),
                Napi::Number::New(env, userId),
                Napi::String::New(env, contentStr) });
  }

  bool OnMpApiEvent(const char* eventName,
                    std::optional<simdjson::dom::element> args,
                    std::optional<uint32_t> formId) override
  {
    if (args && !args->is_array())
      return true;

    auto mp = JsValue::GlobalObject().GetProperty("mp");

    auto f = mp.GetProperty(eventName);
    if (f.GetType() != JsValue::Type::Function)
      return true;

    std::vector<JsValue> argumentsInNapiFormat;
    argumentsInNapiFormat.push_back(JsValue::Undefined()); // Chakra this-arg
    if (formId != std::nullopt) {
      argumentsInNapiFormat.push_back(JsValue::Double(*formId));
    }

    if (args) {
      auto argsArray = args->get_array();
      for (size_t i = 0; i < argsArray.value().size(); ++i) {
        std::string elementString = simdjson::minify(argsArray.value().at(i));
        auto builtinJson = JsValue::GlobalObject().GetProperty("JSON");
        auto parse = builtinJson.GetProperty("parse");
        JsValue resultOfParsing = parse.Call({ builtinJson, elementString });
        argumentsInNapiFormat.push_back(resultOfParsing);
      }
    }

    try {
      auto callResult = f.Call(argumentsInNapiFormat);

      if (callResult.GetType() == JsValue::Type::Undefined)
        return true;

      // TODO: Handle non-boolean values? Current implementation would throw...
      return static_cast<bool>(callResult);
    } catch (std::exception& e) {
      std::cout << "[" << eventName << "] "
                << " " << e.what() << std::endl;
      return true;
    }

    return true;
  }

private:
  ScampServer& server;
};

Napi::FunctionReference ScampServer::constructor;

Napi::Object ScampServer::Init(Napi::Env env, Napi::Object exports)
{
  Napi::Function func = DefineClass(
    env, "ScampServer",
    { InstanceMethod("attachSaveStorage", &ScampServer::AttachSaveStorage),
      InstanceMethod("tick", &ScampServer::Tick),
      InstanceMethod("on", &ScampServer::On),
      InstanceMethod("createActor", &ScampServer::CreateActor),
      InstanceMethod("setUserActor", &ScampServer::SetUserActor),
      InstanceMethod("getUserActor", &ScampServer::GetUserActor),
      InstanceMethod("getActorPos", &ScampServer::GetActorPos),
      InstanceMethod("getActorCellOrWorld", &ScampServer::GetActorCellOrWorld),
      InstanceMethod("getActorName", &ScampServer::GetActorName),
      InstanceMethod("destroyActor", &ScampServer::DestroyActor),
      InstanceMethod("setRaceMenuOpen", &ScampServer::SetRaceMenuOpen),
      InstanceMethod("sendCustomPacket", &ScampServer::SendCustomPacket),
      InstanceMethod("getActorsByProfileId",
                     &ScampServer::GetActorsByProfileId),
      InstanceMethod("setEnabled", &ScampServer::SetEnabled),
      InstanceMethod("createBot", &ScampServer::CreateBot),
      InstanceMethod("getUserByActor", &ScampServer::GetUserByActor),
      InstanceMethod("executeJavaScriptOnChakra",
                     &ScampServer::ExecuteJavaScriptOnChakra),
      InstanceMethod("setSendUiMessageImplementation",
                     &ScampServer::SetSendUiMessageImplementation),
      InstanceMethod("onUiEvent", &ScampServer::OnUiEvent),
      InstanceMethod("clear", &ScampServer::Clear),
      InstanceMethod("writeLogs", &ScampServer::WriteLogs) });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("ScampServer", func);
  return exports;
}

namespace {
std::shared_ptr<IDatabase> CreateDatabase(
  nlohmann::json settings, std::shared_ptr<spdlog::logger> logger)
{
  auto databaseDriver = settings.count("databaseDriver")
    ? settings["databaseDriver"].get<std::string>()
    : std::string("file");

  if (databaseDriver == "file") {
    auto databaseName = settings.count("databaseName")
      ? settings["databaseName"].get<std::string>()
      : std::string("world");

    logger->info("Using file with name '" + databaseName + "'");
    return std::make_shared<FileDatabase>(databaseName, logger);
  }

  if (databaseDriver == "mongodb") {
    auto databaseName = settings.count("databaseName")
      ? settings["databaseName"].get<std::string>()
      : std::string("db");

    auto databaseUri = settings["databaseUri"].get<std::string>();
    logger->info("Using mongodb with name '" + databaseName + "'");
    return std::make_shared<MongoDatabase>(databaseUri, databaseName);
  }

  if (databaseDriver == "migration") {
    auto from = settings.at("databaseOld");
    auto to = settings.at("databaseNew");
    auto oldDatabase = CreateDatabase(from, logger);
    auto newDatabase = CreateDatabase(to, logger);
    return std::make_shared<MigrationDatabase>(newDatabase, oldDatabase);
  }

  throw std::runtime_error("Unrecognized databaseDriver: " + databaseDriver);
}

std::shared_ptr<ISaveStorage> CreateSaveStorage(
  std::shared_ptr<IDatabase> db, std::shared_ptr<spdlog::logger> logger)
{
  return std::make_shared<AsyncSaveStorage>(db, logger);
}

static std::shared_ptr<spdlog::logger>& GetLogger()
{
  static auto g_logger = spdlog::stdout_color_mt("console");
  return g_logger;
}

}

ScampServer::ScampServer(const Napi::CallbackInfo& info)
  : ObjectWrap(info)
  , tickEnv(info.Env())
{
  try {
    partOne = std::make_shared<PartOne>();
    listener = std::make_shared<ScampServerListener>(*this);
    partOne->AddListener(listener);
    Napi::Number port = info[0].As<Napi::Number>(),
                 maxConnections = info[1].As<Napi::Number>();

    serverMock = std::make_shared<Networking::MockServer>();

    std::string dataDir;

    const auto& logger = GetLogger();
    partOne->AttachLogger(logger);

    std::ifstream f("server-settings.json");
    if (!f.good()) {
      throw std::runtime_error("server-settings.json is missing");
    }

    std::stringstream buffer;
    buffer << f.rdbuf();

    auto serverSettings = nlohmann::json::parse(buffer.str());

    if (serverSettings["logLevel"].is_string()) {
      const auto level = spdlog::level::from_str(serverSettings["logLevel"]);
      logger->set_level(level);
      spdlog::set_level(level);
      logger->info("set log level to {}",
                   spdlog::level::to_string_view(logger->level()));
    }

    partOne->worldState.isPapyrusHotReloadEnabled =
      serverSettings.count("isPapyrusHotReloadEnabled") != 0 &&
      serverSettings.at("isPapyrusHotReloadEnabled").get<bool>();
    logger->info("Hot reload is {} for Papyrus",
                 partOne->worldState.isPapyrusHotReloadEnabled ? "enabled"
                                                               : "disabled");

    if (serverSettings["dataDir"] != nullptr) {
      dataDir = serverSettings["dataDir"];
    } else {
      throw std::runtime_error("missing 'dataDir' in server-settings.json");
    }
    logger->info("Using data dir '{}'", dataDir);

    std::vector<std::filesystem::path> pluginPaths = {
      std::filesystem::path(dataDir) / "Skyrim.esm",
      std::filesystem::path(dataDir) / "Update.esm",
      std::filesystem::path(dataDir) / "Dawnguard.esm",
      std::filesystem::path(dataDir) / "HearthFires.esm",
      std::filesystem::path(dataDir) / "Dragonborn.esm"
    };
    if (serverSettings["loadOrder"].is_array()) {
      pluginPaths.clear();
      for (size_t i = 0; i < serverSettings["loadOrder"].size(); ++i) {
        std::filesystem::path loadOrderElement =
          static_cast<std::string>(serverSettings["loadOrder"][i]);
        if (loadOrderElement.is_absolute()) {
          pluginPaths.push_back(loadOrderElement);
        } else {
          pluginPaths.push_back(dataDir / loadOrderElement);
        }
      }
    }

    if (serverSettings["lang"] != nullptr) {
      logger->info("Run localization provider");
      localizationProvider = std::make_shared<LocalizationProvider>(
        serverSettings["dataDir"], serverSettings["lang"]);
    }

    auto scriptStorage = std::make_shared<DirectoryScriptStorage>(
      (espm::fs::path(dataDir) / "scripts").string());

    auto espm = new espm::Loader(pluginPaths);
    auto realServer = Networking::CreateServer(
      static_cast<uint32_t>(port), static_cast<uint32_t>(maxConnections));
    server = Networking::CreateCombinedServer({ realServer, serverMock });
    partOne->SetSendTarget(server.get());
    partOne->SetDamageFormula(std::make_unique<TES5DamageFormula>());
    partOne->worldState.AttachScriptStorage(scriptStorage);
    partOne->AttachEspm(espm);
    this->serverSettings = serverSettings;
    this->logger = logger;

    auto reloot = serverSettings["reloot"];
    for (auto it = reloot.begin(); it != reloot.end(); ++it) {
      std::string recordType = it.key();
      auto timeMs = static_cast<uint64_t>(it.value());
      auto time = std::chrono::milliseconds(1) * timeMs;
      partOne->worldState.SetRelootTime(recordType, time);
      logger->info("'{}' will be relooted every {} ms", recordType, timeMs);
    }

    auto res = RunScript(Env(),
                         "let require = global.require || "
                         "global.process.mainModule.constructor._load; let "
                         "Emitter = require('events'); new Emitter");

    emitter = Napi::Persistent(res.As<Napi::Object>());
    emit = Napi::Persistent(emitter.Value().Get("emit").As<Napi::Function>());
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
}

Napi::Value ScampServer::AttachSaveStorage(const Napi::CallbackInfo& info)
{
  try {
    partOne->AttachSaveStorage(
      CreateSaveStorage(CreateDatabase(serverSettings, logger), logger));
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::Tick(const Napi::CallbackInfo& info)
{
  try {
    tickEnv = info.Env();
    server->Tick(PartOne::HandlePacket, partOne.get());
    partOne->Tick();
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::On(const Napi::CallbackInfo& info)
{
  auto on = emitter.Get("on").As<Napi::Function>();
  on.Call(emitter.Value(), { info[0], info[1] });
  return info.Env().Undefined();
}

Napi::Value ScampServer::CreateActor(const Napi::CallbackInfo& info)
{
  auto formId = info[0].As<Napi::Number>().Uint32Value();
  auto pos = NapiValueToNiPoint3(info[1]);
  auto angleZ = info[2].As<Napi::Number>().FloatValue();
  auto cellOrWorld = info[3].As<Napi::Number>().Uint32Value();

  int32_t userProfileId = -1;
  if (info[4].IsNumber())
    userProfileId = info[4].As<Napi::Number>().Int32Value();
  try {
    uint32_t res =
      partOne->CreateActor(formId, pos, angleZ, cellOrWorld, userProfileId);
    return Napi::Number::New(info.Env(), res);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
}

Napi::Value ScampServer::SetUserActor(const Napi::CallbackInfo& info)
{
  auto userId = info[0].As<Napi::Number>().Uint32Value();
  auto actorFormId = info[1].As<Napi::Number>().Uint32Value();
  try {
    partOne->SetUserActor(userId, actorFormId);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::GetUserActor(const Napi::CallbackInfo& info)
{
  auto userId = info[0].As<Napi::Number>().Uint32Value();
  try {
    return Napi::Number::New(info.Env(), partOne->GetUserActor(userId));
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::GetActorName(const Napi::CallbackInfo& info)
{
  auto actorId = info[0].As<Napi::Number>().Uint32Value();
  try {
    return Napi::String::New(info.Env(), partOne->GetActorName(actorId));
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::GetActorPos(const Napi::CallbackInfo& info)
{
  auto actorId = info[0].As<Napi::Number>().Uint32Value();
  try {
    auto pos = partOne->GetActorPos(actorId);
    auto res = Napi::Array::New(info.Env(), 3);
    for (uint32_t i = 0; i < 3; ++i)
      res.Set(i, Napi::Number::New(info.Env(), pos[i]));
    return res;
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::GetActorCellOrWorld(const Napi::CallbackInfo& info)
{
  auto actorId = info[0].As<Napi::Number>().Uint32Value();
  try {
    return Napi::Number::New(info.Env(),
                             partOne->GetActorCellOrWorld(actorId));
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::DestroyActor(const Napi::CallbackInfo& info)
{
  auto actorFormId = info[0].As<Napi::Number>().Uint32Value();
  try {
    partOne->DestroyActor(actorFormId);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::SetRaceMenuOpen(const Napi::CallbackInfo& info)
{
  auto formId = info[0].As<Napi::Number>().Uint32Value();
  auto open = info[1].As<Napi::Boolean>().operator bool();
  try {
    partOne->SetRaceMenuOpen(formId, open);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::GetActorsByProfileId(const Napi::CallbackInfo& info)
{
  auto profileId = info[0].As<Napi::Number>().Int32Value();
  try {
    auto& actors = partOne->GetActorsByProfileId(profileId);

    auto result = Napi::Array::New(info.Env(), actors.size());
    uint32_t counter = 0;
    for (auto& ac : actors) {
      result.Set(counter, ac);
      ++counter;
    }
    return result;

  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
}

Napi::Value ScampServer::SetEnabled(const Napi::CallbackInfo& info)
{
  auto actorFormId = info[0].As<Napi::Number>().Uint32Value();
  auto enabled = static_cast<bool>(info[1].As<Napi::Boolean>());
  try {
    partOne->SetEnabled(actorFormId, enabled);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::SendCustomPacket(const Napi::CallbackInfo& info)
{
  auto userId = info[0].As<Napi::Number>().Uint32Value();
  auto string = info[1].As<Napi::String>().operator std::string();
  try {
    partOne->SendCustomPacket(userId, string);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::CreateBot(const Napi::CallbackInfo& info)
{
  if (!this->serverMock)
    throw Napi::Error::New(info.Env(), "Bad serverMock");

  auto bot = std::make_shared<Bot>(this->serverMock->CreateClient());

  auto jBot = Napi::Object::New(info.Env());

  jBot.Set(
    "destroy",
    Napi::Function::New(info.Env(), [bot](const Napi::CallbackInfo& info) {
      bot->Destroy();
      return info.Env().Undefined();
    }));
  jBot.Set(
    "send",
    Napi::Function::New(info.Env(), [bot](const Napi::CallbackInfo& info) {
      auto standardJson = info.Env().Global().Get("JSON").As<Napi::Object>();
      auto stringify = standardJson.Get("stringify").As<Napi::Function>();
      std::string s;
      s += Networking::MinPacketId;
      s += (std::string)stringify.Call({ info[0] }).As<Napi::String>();
      bot->Send(s);

      // Memory leak fix
      // TODO: Provide tick API
      bot->Tick();

      return info.Env().Undefined();
    }));

  return jBot;
}

Napi::Value ScampServer::GetUserByActor(const Napi::CallbackInfo& info)
{
  auto formId = info[0].As<Napi::Number>().Uint32Value();
  try {
    return Napi::Number::New(info.Env(), partOne->GetUserByActor(formId));
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

void Err(const Napi::Env& env, std::string msg)
{
  throw Napi::Error::New(env, msg);
}

void Err(const Napi::CallbackInfo& info, std::string msg)
{
  Err(info.Env(), msg);
}

std::string GetPropertyAlphabet()
{
  std::string alphabet;
  for (char c = 'a'; c <= 'z'; c++)
    alphabet += c;
  for (char c = 'A'; c <= 'Z'; c++)
    alphabet += c;
  for (char c = '0'; c <= '9'; c++)
    alphabet += c;
  alphabet += '_';
  return alphabet;
}

uint32_t GetFormId(const JsValue& v)
{
  if (v.GetType() == JsValue::Type::Number) {
    double formId = static_cast<double>(v);
    constexpr auto max =
      static_cast<double>(std::numeric_limits<uint32_t>::max());
    if (std::isfinite(formId) && formId >= 0 && formId < max) {
      return static_cast<uint32_t>(floor(formId));
    }
  }
  return 0;
}

std::string ExtractString(const JsValue& v, const char* argName)
{
  if (v.GetType() != JsValue::Type::String) {
    std::stringstream ss;
    ss << "Expected '" << argName << "' to be string, but got '";
    ss << v.ToString();
    ss << "'";
    throw std::runtime_error(ss.str());
  }
  return static_cast<std::string>(v);
}

const JsValue& ExtractFunction(const JsValue& v, const char* argName)
{
  if (v.GetType() != JsValue::Type::Function) {
    std::stringstream ss;
    ss << "Expected '" << argName << "' to be function, but got '";
    ss << v.ToString();
    ss << "'";
    throw std::runtime_error(ss.str());
  }
  return v;
}

uint32_t ExtractFormId(const JsValue& v, const char* argName = "formId")
{
  if (v.GetType() != JsValue::Type::Number) {
    std::stringstream ss;
    ss << "Expected '" << argName << "' to be number, but got '";
    ss << v.ToString();
    ss << "'";
    throw std::runtime_error(ss.str());
  }
  return GetFormId(v);
}

std::string ExtractNewValueStr(Napi::Value v)
{
  auto builtinJson = v.Env().Global().Get("JSON").As<Napi::Object>();
  auto stringify = builtinJson.Get("stringify").As<Napi::Function>();
  std::string dump = stringify.Call(builtinJson, { v }).As<Napi::String>();
  return dump;
}

std::string ExtractNewValueStr(const JsValue& v)
{
  auto builtinJson = JsValue::GlobalObject().GetProperty("JSON");
  auto stringify = builtinJson.GetProperty("stringify");
  return stringify.Call({ builtinJson, v });
}

nlohmann::json ExtractNewValue(Napi::Value v)
{
  return nlohmann::json::parse(ExtractNewValueStr(v));
}

nlohmann::json ExtractNewValue(const JsValue& v)
{
  return nlohmann::json::parse(ExtractNewValueStr(v));
}

void EnsurePropertyExists(const GamemodeApi::State& state,
                          const std::string& propertyName)
{
  if (!state.createdProperties.count(propertyName)) {
    std::stringstream ss;
    ss << "Property '" << propertyName << "' doesn't exist";
    throw std::runtime_error(ss.str());
  }
}

Napi::Value ParseJson(const Napi::Env& env, const std::string& dump)
{
  auto builtinJson = env.Global().Get("JSON").As<Napi::Object>();
  auto parse = builtinJson.Get("parse").As<Napi::Function>();
  return parse.Call({ Napi::String::New(env, dump) });
}

JsValue ParseJsonChakra(const std::string& dump)
{
  auto builtinJson = JsValue::GlobalObject().GetProperty("JSON");
  auto parse = builtinJson.GetProperty("parse");
  return parse.Call({ builtinJson, JsValue(dump) });
}

JsValue GetJsObjectFromPapyrusObject(
  const VarValue& value, const std::vector<std::string>& espmFilenames)
{
  auto ptr = static_cast<IGameObject*>(value);
  if (!ptr) {
    return JsValue::Null();
  }

  if (auto concrete = dynamic_cast<EspmGameObject*>(ptr)) {
    auto rawId = concrete->record.rec->GetId();
    auto id = concrete->record.ToGlobalId(rawId);

    auto desc = FormDesc::FromFormId(id, espmFilenames).ToString();

    auto result = JsValue::Object();
    result.SetProperty("type", JsValue("espm"));
    result.SetProperty("desc", JsValue(desc));
    return result;
  }

  if (auto concrete = dynamic_cast<MpFormGameObject*>(ptr)) {
    auto formId =
      concrete->GetFormPtr() ? concrete->GetFormPtr()->GetFormId() : 0;

    auto desc = FormDesc::FromFormId(formId, espmFilenames).ToString();

    auto result = JsValue::Object();
    result.SetProperty("type", JsValue("form"));
    result.SetProperty("desc", JsValue(desc));
    return result;
  }

  throw std::runtime_error("This type of IGameObject is not supported in JS");
}

JsValue GetJsValueFromPapyrusValue(
  const VarValue& value, const std::vector<std::string>& espmFilenames)
{
  if (value.promise) {
    auto promiseCallback = JsValue::Function(
      [value, espmFilenames](const JsFunctionArguments& args) {
        auto& resolve = args[1];

        value.promise->Then([resolve, espmFilenames](const VarValue& v) {
          resolve.Call({ JsValue::Undefined(),
                         GetJsValueFromPapyrusValue(v, espmFilenames) });
        });

        // TODO: catch/reject?

        return JsValue::Undefined();
      });
  }
  switch (value.GetType()) {
    case VarValue::kType_Object:
      return GetJsObjectFromPapyrusObject(value, espmFilenames);
    case VarValue::kType_Identifier:
      throw std::runtime_error(
        "Unexpected convertion from Papyrus identifier");
    case VarValue::kType_String: {
      std::string str = static_cast<const char*>(value);
      return JsValue::String(str);
    }
    case VarValue::kType_Integer: {
      auto v = static_cast<int32_t>(value);
      return JsValue::Int(v);
    }
    case VarValue::kType_Float: {
      auto v = static_cast<double>(value);
      return JsValue::Double(v);
    }
    case VarValue::kType_Bool: {
      auto v = static_cast<bool>(value);
      return JsValue::Bool(v);
    }

    case VarValue::kType_ObjectArray:
    case VarValue::kType_StringArray:
    case VarValue::kType_IntArray:
    case VarValue::kType_FloatArray:
    case VarValue::kType_BoolArray: {
      if (value.pArray == nullptr)
        return JsValue::Null();
      auto arr = JsValue::Array(value.pArray->size());
      int n = static_cast<int>(arr.GetProperty("length"));
      for (int i = 0; i < n; ++i) {
        arr.SetProperty(
          i, GetJsValueFromPapyrusValue(value.pArray->at(i), espmFilenames));
      }
      return arr;
    }
  }
  std::stringstream ss;
  ss << "Could not convert a Papyrus value " << value << " to JS format";
  throw std::runtime_error(ss.str());
}

VarValue GetPapyrusValueFromJsValue(const JsValue& v, bool treatNumberAsInt,
                                    WorldState& wst)
{
  switch (v.GetType()) {
    case JsValue::Type::Boolean:
      if (std::string(v.ToString())[0] == 't') {
        return VarValue(true);
      }
      return VarValue(false);
    case JsValue::Type::Null:
      return VarValue::None();
    // undefined is not a valid value in Papyrus
    // But TypeScript should be able to return void, so:
    case JsValue::Type::Undefined: {
      return VarValue::None();
    }
    case JsValue::Type::Number: {
      double number = static_cast<double>(v);
      return treatNumberAsInt ? VarValue(static_cast<int32_t>(number))
                              : VarValue(number);
    }
    case JsValue::Type::String: {
      auto str = static_cast<std::string>(v);
      VarValue res(str);
      return res;
    }
    case JsValue::Type::Array: {
      auto arr = v;
      if (arr.GetProperty("length").ToString() == "0") {
        // Treat zero-length arrays as kType_ObjectArray ("none array")
        VarValue papyrusArray(VarValue::kType_ObjectArray);
        papyrusArray.pArray.reset(new std::vector<VarValue>);
        return papyrusArray;
      }

      auto arrayContents = std::make_shared<std::vector<VarValue>>();
      uint8_t type = ~0;

      int n = static_cast<int>(arr.GetProperty("length"));
      for (int i = 0; i < n; ++i) {
        arrayContents->push_back(GetPapyrusValueFromJsValue(
          arr.GetProperty(i), treatNumberAsInt, wst));

        auto extractedType = arrayContents->back().GetType();
        if (type == static_cast<uint8_t>(~0)) {
          type = extractedType;
        } else if (extractedType != type) {
          throw std::runtime_error(
            "Papyrus doesn't support heterogeneous arrays");
        }
      }

      VarValue papyrusArray(
        ActivePexInstance::GetArrayTypeByElementType(type));
      papyrusArray.pArray = arrayContents;

      return papyrusArray;
    }
    case JsValue::Type::Object: {
      bool isPromise =
        v.GetProperty("then").GetType() == JsValue::Type::Function;
      if (isPromise) {
        VarValue res = VarValue::None();
        res.promise = std::make_shared<Viet::Promise<VarValue>>();

        auto then = v.GetProperty("then");

        auto wst_ = &wst;
        then.Call(
          { v, JsValue::Function([res, wst_](const JsFunctionArguments& args) {
              bool treatNumberAsInt = false;
              res.promise->Resolve(
                GetPapyrusValueFromJsValue(args[1], treatNumberAsInt, *wst_));
              return JsValue::Undefined();
            }) });
        // TODO: catch/reject?

        return res;
      }

      auto desc = static_cast<std::string>(v.GetProperty("desc"));
      auto type = static_cast<std::string>(v.GetProperty("type"));

      const auto espmFileNames = wst.GetEspm().GetFileNames();
      uint32_t id = FormDesc::FromString(desc).ToFormId(espmFileNames);

      if (type == "form") {
        MpObjectReference& refr = wst.GetFormAt<MpObjectReference>(id);
        return VarValue(std::make_shared<MpFormGameObject>(&refr));
      }

      if (type == "espm") {
        auto lookupRes = wst.GetEspm().GetBrowser().LookupById(id);
        if (!lookupRes.rec) {
          std::stringstream ss;
          ss << "ESPM record with id " << std::hex << id << " doesn't exist";
          throw std::runtime_error(ss.str());
        }
        return VarValue(std::make_shared<EspmGameObject>(lookupRes));
      }

      std::stringstream ss;
      ss << "Unknown object type '" << type << "', must be 'form' | 'espm'";
      throw std::runtime_error(ss.str());
    }
    default:
      break;
  }
  std::stringstream ss;
  ss << "JS type " << static_cast<int>(v.GetType())
     << " is not castable to any of Papyrus types";
  throw std::runtime_error(ss.str());
}

std::string GetDataDirSafe(nlohmann::json serverSettings)
{
  std::string dataDir = serverSettings["dataDir"];
  if (dataDir != "data") {
    // Don't want to deal with security issues, so only <server_root>/data
    // please
    throw std::runtime_error(
      "readDataDirectory doesn't support custom dataDir in "
      "server-settings.json, consider using 'data' as dataDir");
  }
  return dataDir;
}

void ScampServer::RegisterChakraApi(std::shared_ptr<JsEngine> chakraEngine)
{
  JsValue mp = JsValue::Object();

  mp.SetProperty(
    "getLocalizedString",
    JsValue::Function([this](const JsFunctionArguments& args) {
      auto translatedString = JsValue::Undefined();

      if (!localizationProvider) {
        return translatedString;
      }

      auto globalRecordId = ExtractFormId(args[1], "globalRecordId");
      auto lookupRes =
        partOne->GetEspm().GetBrowser().LookupById(globalRecordId);

      if (!lookupRes.rec) {
        return translatedString;
      }

      auto fields = JsValue::Array(0);

      auto& cache = partOne->worldState.GetEspmCache();

      espm::IterateFields_(
        lookupRes.rec,
        [&](const char* type, uint32_t size, const char* data) {
          if (std::string(type, 4) != "FULL" || size != 4) {
            return;
          }

          auto stringId = *reinterpret_cast<const uint32_t*>(data);
          if (!serverSettings["loadOrder"].is_array()) {
            return;
          }

          for (size_t i = 0; i < serverSettings["loadOrder"].size(); ++i) {
            if (i != lookupRes.fileIdx) {
              continue;
            }

            std::filesystem::path loadOrderElement =
              static_cast<std::string>(serverSettings["loadOrder"][i]);

            auto fileNameFull = loadOrderElement.filename().string();
            auto fileNameWithoutExt =
              fileNameFull.substr(0, fileNameFull.find_last_of("."));

            std::transform(fileNameWithoutExt.begin(),
                           fileNameWithoutExt.end(),
                           fileNameWithoutExt.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            translatedString = JsValue::String(
              this->localizationProvider->Get(fileNameWithoutExt, stringId));
          }
        },
        cache);

      return translatedString;
    }));

  mp.SetProperty("getServerSettings",
                 JsValue::Function([this](const JsFunctionArguments& args) {
                   auto builtinJson =
                     JsValue::GlobalObject().GetProperty("JSON");
                   auto builtinParse = builtinJson.GetProperty("parse");
                   return builtinParse.Call(
                     { builtinJson, JsValue(serverSettings.dump()) });
                 }));

  mp.SetProperty(
    "readDataDirectory",
    JsValue::Function([this](const JsFunctionArguments& args) {
      auto dataDir = GetDataDirSafe(serverSettings);
      std::vector<JsValue> paths;
      for (std::filesystem::recursive_directory_iterator i(dataDir), end;
           i != end; ++i) {
        std::string p = i->path().string();

        // Remove "data/" prefix to be consistent with readDataFile
        p = std::string{ p.begin() + dataDir.size() + 1, p.end() };

        paths.push_back(p);
      }
      return paths;
    }));

  mp.SetProperty("readDataFile",
                 JsValue::Function([this](const JsFunctionArguments& args) {
                   std::string path = args[1];
                   if (path.find("..") != std::string::npos) {
                     throw std::runtime_error(
                       "readDataFile doesn't support paths containing '..'");
                   }
                   auto dataDir = GetDataDirSafe(serverSettings);
                   auto filePath = std::filesystem::path(dataDir) / path;

                   std::ifstream t(filePath);
                   std::stringstream buffer;
                   buffer << t.rdbuf();
                   return buffer.str();
                 }));

  mp.SetProperty("writeDataFile",
                 JsValue::Function([this](const JsFunctionArguments& args) {
                   std::string path = args[1];

                   if (path.find("..") != std::string::npos) {
                     throw std::runtime_error(
                       "writeDataFile doesn't support paths containing '..'");
                   }

                   auto dataDir = GetDataDirSafe(serverSettings);
                   auto filePath = std::filesystem::path(dataDir) / path;

                   std::string stringToWrite = args[2];
                   std::ofstream dataFile(filePath);

                   dataFile << stringToWrite;

                   dataFile.close();

                   return JsValue::Undefined();
                 }));

  auto update = [this] {
    partOne->NotifyGamemodeApiStateChanged(gamemodeApiState);
  };

  mp.SetProperty(
    "clear",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      gamemodeApiState = GamemodeApi::State();
      update();
      return JsValue::Undefined();
    }));

  mp.SetProperty(
    "makeProperty",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto propertyName = ExtractString(args[1], "propertyName");
      if (propertyName.size() < 1 || propertyName.size() > 128) {
        std::stringstream ss;
        ss << "The length of 'propertyName' must be between 1 and 128, but "
              "it "
              "is '";
        ss << propertyName.size();
        ss << "'";
        throw std::runtime_error(ss.str());
      }

      auto alphabet = GetPropertyAlphabet();
      if (propertyName.find_first_not_of(alphabet.data()) !=
          std::string::npos) {
        std::stringstream ss;
        ss << "'propertyName' may contain only Latin characters, numbers, "
              "and underscore";
        throw std::runtime_error(ss.str());
      }

      if (gamemodeApiState.createdProperties.count(propertyName)) {
        std::stringstream ss;
        ss << "'propertyName' must be unique";
        throw std::runtime_error(ss.str());
      }

      GamemodeApi::PropertyInfo propertyInfo;

      if (args[2].GetType() != JsValue::Type::Object) {
        std::stringstream ss;
        ss << "Expected 'options' to be object, but got '";
        ss << args[2].ToString();
        ss << "'";
        throw std::runtime_error(ss.str());
      }

      auto options = args[2];

      std::vector<std::pair<std::string, bool*>> booleans{
        { "isVisibleByOwner", &propertyInfo.isVisibleByOwner },
        { "isVisibleByNeighbors", &propertyInfo.isVisibleByNeighbors }
      };
      for (auto [optionName, ptr] : booleans) {
        auto v = options.GetProperty(optionName.data());
        if (v.GetType() != JsValue::Type::Boolean) {
          std::stringstream ss;
          ss << "Expected 'options." << optionName;
          ss << "' to be boolean, but got '";
          ss << v.ToString();
          ss << "'";
          throw std::runtime_error(ss.str());
        }
        *ptr = static_cast<bool>(v);
      }

      std::vector<std::pair<std::string, std::string*>> strings{
        { "updateNeighbor", &propertyInfo.updateNeighbor },
        { "updateOwner", &propertyInfo.updateOwner }
      };
      for (auto [optionName, ptr] : strings) {
        auto v = options.GetProperty(optionName.data());
        if (v.GetType() != JsValue::Type::String) {
          std::stringstream ss;
          ss << "Expected 'options." << optionName;
          ss << "' to be string, but got '";
          ss << v.ToString();
          ss << "'";
          throw std::runtime_error(ss.str());
        }
        *ptr = static_cast<std::string>(v);
      }

      gamemodeApiState.createdProperties[propertyName] = propertyInfo;

      update();
      return JsValue::Undefined();
    }));

  mp.SetProperty(
    "makeEventSource",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      if (args[1].GetType() != JsValue::Type::String) {
        std::stringstream ss;
        ss << "Expected 'eventName' to be string, but got '";
        ss << args[1].ToString();
        ss << "'";
        throw std::runtime_error(ss.str());
      }

      auto eventName = static_cast<std::string>(args[1]);

      if (gamemodeApiState.createdEventSources.count(eventName)) {
        std::stringstream ss;
        ss << "'eventName' must be unique";
        throw std::runtime_error(ss.str());
      }

      if (args[2].GetType() != JsValue::Type::String) {
        std::stringstream ss;
        ss << "Expected 'functionBody' to be string, but got '";
        ss << args[2].ToString();
        ss << "'";
        throw std::runtime_error(ss.str());
      }

      auto functionBody = static_cast<std::string>(args[2]);
      gamemodeApiState.createdEventSources[eventName] = { functionBody };

      update();
      return JsValue::Undefined();
    }));

  mp.SetProperty(
    "get", JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto propertyName = ExtractString(args[2], "propertyName");
      auto formId = ExtractFormId(args[1]);

      // Global properties
      if (formId == 0 && propertyName == "onlinePlayers") {

        auto n = partOne->serverState.userInfo.size();
        std::vector<uint32_t> ids;
        ids.reserve(n);

        for (size_t i = 0; i < n; ++i) {
          if (auto actor = partOne->serverState.ActorByUser(i)) {
            ids.push_back(actor->GetFormId());
          }
        }

        auto arr = JsValue::Array(ids.size());
        int i = 0;
        for (auto id : ids) {
          arr.SetProperty(JsValue(i), JsValue(static_cast<double>(id)));
          ++i;
        }
        return arr;
      }

      auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

      JsValue res = JsValue::Undefined();

      if (propertyName == "type") {
        if (dynamic_cast<MpActor*>(&refr)) {
          res = JsValue("MpActor");
        } else {
          res = JsValue("MpObjectReference");
        }
      } else if (propertyName == "pos" || propertyName == "angle") {
        auto niPoint3 =
          propertyName == "pos" ? refr.GetPos() : refr.GetAngle();
        auto arr = JsValue::Array(3);
        for (int i = 0; i < 3; ++i) {
          arr.SetProperty(JsValue(i), JsValue(niPoint3[i]));
        }
        res = arr;
      } else if (propertyName == "worldOrCellDesc") {
        auto desc = refr.GetCellOrWorld();
        res = JsValue(desc.ToString());
      } else if (propertyName == "baseDesc") {
        auto desc = FormDesc::FromFormId(refr.GetBaseId(),
                                         partOne->worldState.espmFiles);
        res = JsValue(desc.ToString());
      } else if (propertyName == "isOpen") {
        res = JsValue(refr.IsOpen());
      } else if (propertyName == "appearance") {
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          auto& dump = actor->GetAppearanceAsJson();
          if (dump.size() > 0) {
            res = ParseJsonChakra(dump);
          }
        }
      } else if (propertyName == "inventory") {
        res = ParseJsonChakra(refr.GetInventory().ToJson().dump());
      } else if (propertyName == "equipment") {
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          auto& dump = actor->GetEquipmentAsJson();
          if (dump.size() > 0) {
            res = ParseJsonChakra(dump);
          }
        }
      } else if (propertyName == "isOnline") {
        res = JsValue(false);
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          auto userId = partOne->serverState.UserByActor(actor);
          if (userId != Networking::InvalidUserId) {
            res = JsValue(true);
          }
        }
      } else if (propertyName == "formDesc") {
        auto desc = FormDesc::FromFormId(refr.GetFormId(),
                                         partOne->worldState.espmFiles);
        res = JsValue(desc.ToString());
      } else if (propertyName == "neighbors" ||
                 propertyName == "actorNeighbors") {
        std::set<MpObjectReference*> ids;

        if (propertyName == "actorNeighbors") {
          for (auto listener : refr.GetListeners()) {
            ids.insert(dynamic_cast<MpActor*>(listener));
          }
          for (auto emitter : refr.GetEmitters()) {
            ids.insert(dynamic_cast<MpActor*>(emitter));
          }
          ids.erase(nullptr);
        } else {
          for (auto listener : refr.GetListeners()) {
            ids.insert(listener);
          }
          for (auto emitter : refr.GetEmitters()) {
            ids.insert(emitter);
          }
        }

        auto arr = JsValue::Array(ids.size());
        int i = 0;
        for (auto id : ids) {
          arr.SetProperty(JsValue(i),
                          JsValue(static_cast<double>(id->GetFormId())));
          ++i;
        }
        res = arr;
      } else if (propertyName == "isDisabled") {
        res = JsValue(refr.IsDisabled());
      } else if (propertyName == "isDead") {
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          res = JsValue::Bool(actor->IsDead());
        }
      } else if (propertyName == "percentages") {
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          auto chForm = actor->GetChangeForm();
          res = JsValue::Object();
          res.SetProperty("health", chForm.healthPercentage);
          res.SetProperty("magicka", chForm.magickaPercentage);
          res.SetProperty("stamina", chForm.staminaPercentage);
        }
      } else if (propertyName == "profileId") {
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          auto chForm = actor->GetChangeForm();
          res = JsValue::Int(chForm.profileId);
        }
      } else {
        EnsurePropertyExists(gamemodeApiState, propertyName);
        res = refr.GetDynamicFields().Get(propertyName);
      }

      return res;
    }));

  mp.SetProperty(
    "set", JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto formId = ExtractFormId(args[1]);
      auto propertyName = ExtractString(args[2], "propertyName");
      auto newValue = ExtractNewValue(args[3]);
      auto newValueChakra = args[3];

      auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

      if (propertyName == "locationalData" || propertyName == "spawnPoint") {
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          LocationalData locationalData;
          locationalData.cellOrWorldDesc =
            FormDesc::FromString(newValue["cellOrWorldDesc"]);
          for (int i = 0; i < 3; ++i) {
            locationalData.pos[i] = newValue["pos"][i].get<float>();
            locationalData.rot[i] = newValue["rot"][i].get<float>();
          }
          if (propertyName == "locationalData") {
            actor->Teleport(locationalData);
          } else {
            actor->SetSpawnPoint(locationalData);
          }
        } else {
          throw std::runtime_error("mp.set can only change 'locationalData' "
                                   "for actors, not for refrs");
        }
      } else if (propertyName == "isOpen") {
        refr.SetOpen(newValue.get<bool>());
      } else if (propertyName == "appearance") {
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          // TODO: Live update of appearance
          if (newValue.is_object()) {
            auto appearance = Appearance::FromJson(newValue);
            actor->SetAppearance(&appearance);
          } else {
            actor->SetAppearance(nullptr);
          }
        }
      } else if (propertyName == "inventory") {
        if (newValue.is_object()) {
          auto inv = Inventory::FromJson(newValue);
          refr.SetInventory(inv);
        } else {
          refr.SetInventory(Inventory());
        }
      } else if (propertyName == "equipment") {
        // TODO: Implement this
        throw std::runtime_error("mp.set is not implemented for 'equipment'");
      } else if (propertyName == "isOnline") {
        throw std::runtime_error("mp.set is not implemented for 'isOnline'");
      } else if (propertyName == "formDesc") {
        throw std::runtime_error("mp.set is not implemented for 'formDesc'");
      } else if (propertyName == "onlinePlayers") {
        throw std::runtime_error(
          "mp.set is not implemented for 'onlinePlayers'");
      } else if (propertyName == "neighbors") {
        throw std::runtime_error("mp.set is not implemented for 'neighbors'");
      } else if (propertyName == "isDisabled") {
        if (refr.GetFormId() < 0xff000000)
          throw std::runtime_error(
            "'isDisabled' is not usable for non-FF forms");
        newValue.get<bool>() ? refr.Disable() : refr.Enable();
      } else if (propertyName == "isDead") {
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          actor->SetIsDead(newValue.get<bool>());
        }
      } else if (propertyName == "percentages") {
        if (auto actor = dynamic_cast<MpActor*>(&refr)) {
          actor->NetSetPercentages(newValue["health"].get<float>(),
                                   newValue["magicka"].get<float>(),
                                   newValue["stamina"].get<float>());
        }
      } else {

        EnsurePropertyExists(gamemodeApiState, propertyName);

        auto& info = gamemodeApiState.createdProperties[propertyName];

        refr.SetProperty(propertyName, newValue, newValueChakra,
                         info.isVisibleByOwner, info.isVisibleByNeighbors);
      }
      return JsValue::Undefined();
    }));

  mp.SetProperty(
    "place",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto globalRecordId = ExtractFormId(args[1], "globalRecordId");
      auto akFormToPlace =
        partOne->GetEspm().GetBrowser().LookupById(globalRecordId);
      if (!akFormToPlace.rec) {
        std::stringstream ss;
        ss << std::hex << "Bad record Id " << globalRecordId;
        throw std::runtime_error(ss.str());
      }

      std::string type = akFormToPlace.rec->GetType().ToString();

      LocationalData locationalData = { { 0, 0, 0 },
                                        { 0, 0, 0 },
                                        FormDesc::Tamriel() };
      FormCallbacks callbacks = partOne->CreateFormCallbacks();

      std::unique_ptr<MpObjectReference> newRefr;
      if (akFormToPlace.rec->GetType() == "NPC_") {
        auto actor = new MpActor(locationalData, callbacks, globalRecordId);
        newRefr.reset(actor);
      } else {
        newRefr.reset(new MpObjectReference(locationalData, callbacks,
                                            globalRecordId, type));
      }

      auto worldState = &partOne->worldState;
      auto newRefrId = worldState->GenerateFormId();
      worldState->AddForm(std::move(newRefr), newRefrId);

      auto& refr = worldState->GetFormAt<MpObjectReference>(newRefrId);
      refr.ForceSubscriptionsUpdate();

      return JsValue(static_cast<double>(refr.GetFormId()));
    }));

  mp.SetProperty(
    "lookupEspmRecordById",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto globalRecordId = ExtractFormId(args[1], "globalRecordId");

      auto espmLookupResult = JsValue::Object();

      auto lookupRes =
        partOne->GetEspm().GetBrowser().LookupById(globalRecordId);
      if (lookupRes.rec) {
        auto fields = JsValue::Array(0);

        auto& cache = partOne->worldState.GetEspmCache();

        espm::IterateFields_(
          lookupRes.rec,
          [&](const char* type, uint32_t size, const char* data) {
            auto uint8arr = JsValue::Uint8Array(size);
            memcpy(uint8arr.GetTypedArrayData(), data, size);

            auto push = fields.GetProperty("push");
            auto field = JsValue::Object();
            field.SetProperty("type", JsValue(std::string(type, 4)));
            field.SetProperty("data", uint8arr);
            push.Call({ fields, field });
          },
          cache);

        auto id = JsValue(static_cast<double>(lookupRes.rec->GetId()));
        auto edid = JsValue(lookupRes.rec->GetEditorId(cache));
        auto type = JsValue(lookupRes.rec->GetType().ToString());
        auto flags = JsValue(static_cast<double>(lookupRes.rec->GetFlags()));

        auto record = JsValue::Object();
        record.SetProperty("id", id);
        record.SetProperty("editorId", edid);
        record.SetProperty("type", type);
        record.SetProperty("flags", flags);
        record.SetProperty("fields", fields);
        espmLookupResult.SetProperty("record", record);

        espmLookupResult.SetProperty(
          "fileIndex", JsValue(static_cast<int>(lookupRes.fileIdx)));
      }

      return espmLookupResult;
    }));

  mp.SetProperty(
    "getEspmLoadOrder",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto fileNames = partOne->GetEspm().GetFileNames();
      auto arr = JsValue::Array(fileNames.size());
      for (int i = 0; i < static_cast<int>(fileNames.size()); ++i) {
        arr.SetProperty(JsValue(i), JsValue(fileNames[i]));
      }
      return arr;
    }));

  mp.SetProperty(
    "getDescFromId",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto formId = ExtractFormId(args[1]);
      auto espmFileNames = partOne->GetEspm().GetFileNames();
      auto formDesc = FormDesc::FromFormId(formId, espmFileNames);

      return JsValue(formDesc.ToString());
    }));

  mp.SetProperty(
    "getIdFromDesc",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto str = ExtractString(args[1], "formDesc");
      auto espmFileNames = partOne->GetEspm().GetFileNames();
      auto formDesc = FormDesc::FromString(str);

      return JsValue(static_cast<double>(formDesc.ToFormId(espmFileNames)));
    }));

  mp.SetProperty(
    "callPapyrusFunction",
    JsValue::Function([this, update](const JsFunctionArguments& chakraArgs) {
      auto callType = ExtractString(chakraArgs[1], "callType");
      auto className = ExtractString(chakraArgs[2], "className");
      auto functionName = ExtractString(chakraArgs[3], "functionName");
      auto self =
        GetPapyrusValueFromJsValue(chakraArgs[4], false, partOne->worldState);

      auto arr = chakraArgs[5];
      auto arrSize = static_cast<int>(arr.GetProperty("length"));

      bool treatNumberAsInt = false; // TODO?

      std::vector<VarValue> args;
      args.resize(arrSize);
      for (int i = 0; i < arrSize; ++i) {
        args[i] = GetPapyrusValueFromJsValue(
          arr.GetProperty(JsValue(i)), treatNumberAsInt, partOne->worldState);
      }

      VarValue res;

      auto& vm = partOne->worldState.GetPapyrusVm();
      if (callType == "method") {
        res = vm.CallMethod(static_cast<IGameObject*>(self),
                            functionName.data(), args);
      } else if (callType == "global") {
        res = vm.CallStatic(className, functionName, args);
      } else {

        throw std::runtime_error("Unknown callType " + callType);
      }

      return GetJsValueFromPapyrusValue(res, partOne->worldState.espmFiles);
    }));

  mp.SetProperty(
    "registerPapyrusFunction",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto callType = ExtractString(args[1], "callType");
      auto className = ExtractString(args[2], "className");
      auto functionName = ExtractString(args[3], "functionName");
      auto& f = ExtractFunction(args[4], "f");

      auto& vm = partOne->worldState.GetPapyrusVm();

      auto* wst = &partOne->worldState;

      FunctionType fType;

      if (callType == "method") {
        fType = FunctionType::Method;
      } else if (callType == "global") {
        fType = FunctionType::GlobalFunction;
      } else {

        throw std::runtime_error("Unknown callType " + callType);
      }

      vm.RegisterFunction(
        className, functionName, fType,
        [f, wst](const VarValue& self, const std::vector<VarValue>& args) {
          JsValue jsSelf = GetJsValueFromPapyrusValue(self, wst->espmFiles);

          auto jsArgs = JsValue::Array(static_cast<uint32_t>(args.size()));
          for (int i = 0; i < static_cast<int>(args.size()); ++i) {
            jsArgs.SetProperty(
              i, GetJsValueFromPapyrusValue(args[i], wst->espmFiles));
          }

          auto jsResult = f.Call({ JsValue::Undefined(), jsSelf, jsArgs });

          bool treatResultAsInt = false; // TODO?

          return GetPapyrusValueFromJsValue(jsResult, treatResultAsInt, *wst);
        });

      return JsValue::Undefined();
    }));

  mp.SetProperty(
    "sendUiMessage",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto formId = ExtractFormId(args[1]);
      std::string msgDump = ExtractNewValueStr(args[2]);
      auto env = sendUiMessageImplementation.Env();
      std::vector<napi_value> sendUiMessageArgs;
      sendUiMessageArgs.push_back(Napi::Number::New(env, formId));
      sendUiMessageArgs.push_back(ParseJson(env, msgDump));
      sendUiMessageImplementation.Call(sendUiMessageArgs);
      return JsValue::Undefined();
    }));

  mp.SetProperty(
    "sendCustomPacket",
    JsValue::Function([this, update](const JsFunctionArguments& args) {
      auto formId = ExtractFormId(args[1]);
      std::string packet = ExtractNewValueStr(args[2]);
      auto userId = partOne->GetUserByActor(formId);
      partOne->SendCustomPacket(userId, packet);
      return JsValue::Undefined();
    }));

  JsValue::GlobalObject().SetProperty("mp", mp);

  JsValue console = JsValue::Object();

  console.SetProperty(
    "log", JsValue::Function([this](const JsFunctionArguments& args) {
      std::string s;

      for (size_t i = 1; i < args.GetSize(); ++i) {
        JsValue str = args[i];
        if (args[i].GetType() == JsValue::Type::Object &&
            !args[i].GetExternalData()) {

          JsValue json = JsValue::GlobalObject().GetProperty("JSON");
          str = json.GetProperty("stringify").Call({ json, args[i] });
        }
        s += str.ToString() + ' ';
      }

      auto console = Env().Global().Get("console").As<Napi::Object>();
      auto log = console.Get("log").As<Napi::Function>();
      log.Call(console, { Napi::String::New(Env(), s) });

      return JsValue::Undefined();
    }));
  JsValue::GlobalObject().SetProperty("console", console);

  auto setTimeout = JsValue::Function([this](const JsFunctionArguments& args) {
    auto& f = ExtractFunction(args[1], "callback");
    int ms = static_cast<int>(args[2]);

    float seconds = ms / 1000.f;

    partOne->worldState.SetTimer(seconds).Then(
      [f](Viet::Void) { f.Call({}); });

    return JsValue::Undefined();
  });

  JsValue::GlobalObject().SetProperty("setTimeout", setTimeout);

  auto textDecoder =
    JsValue::Function([this](const JsFunctionArguments& args) {
      auto self = args[0];
      self.SetProperty(
        "decode", JsValue::Function([this](const JsFunctionArguments& args) {
          int n = static_cast<int>(args[1].GetProperty("length"));
          std::string src = "require('fs').writeFileSync('kek', new "
                            "TextDecoder('utf-8').decode(new Uint8Array([";
          for (int i = 0; i < n; ++i) {
            int byte = static_cast<int>(args[1].GetProperty(JsValue(i)));
            if (i) {
              src += ", ";
            }
            src += std::to_string(byte);
          }
          src += "]))    )";

          RunScript(Env(), src);

          std::ifstream t("kek");
          std::stringstream buffer;
          buffer << t.rdbuf();

          return JsValue::String(buffer.str());
        }));
      return self;
    });

  JsValue::GlobalObject().SetProperty("TextDecoder", textDecoder);
}

Napi::Value ScampServer::ExecuteJavaScriptOnChakra(
  const Napi::CallbackInfo& info)
{
  try {
    if (!chakraEngine) {
      chakraEngine.reset(new JsEngine);
      chakraEngine->ResetContext(chakraTaskQueue);
    }
    auto src = static_cast<std::string>(info[0].As<Napi::String>());

    RegisterChakraApi(chakraEngine);

    chakraEngine->RunScript(src, "skymp5-gamemode/gamemode.js");
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::SetSendUiMessageImplementation(
  const Napi::CallbackInfo& info)
{
  try {
    Napi::Function fn = info[0].As<Napi::Function>();
    sendUiMessageImplementation = Napi::Persistent(fn);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::OnUiEvent(const Napi::CallbackInfo& info)
{
  try {
    Napi::Number jsFormId = info[0].As<Napi::Number>();
    Napi::Object jsMsg = info[1].As<Napi::Object>();

    double formId = static_cast<double>(jsFormId);
    std::string msg = ExtractNewValueStr(jsMsg);

    if (!chakraEngine) {
      throw std::runtime_error("nullptr chakraEngine");
    }

    auto builtinJson = JsValue::GlobalObject().GetProperty("JSON");
    auto builtinParse = builtinJson.GetProperty("parse");
    JsValue chakraMsg = builtinParse.Call({ builtinJson, JsValue(msg) });

    auto onUiEvent =
      JsValue::GlobalObject().GetProperty("mp").GetProperty("onUiEvent");
    onUiEvent.Call({ JsValue::Undefined(), JsValue(formId), chakraMsg });

  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::Clear(const Napi::CallbackInfo& info)
{
  try {
    gamemodeApiState = GamemodeApi::State();
    partOne->NotifyGamemodeApiStateChanged(gamemodeApiState);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::WriteLogs(const Napi::CallbackInfo& info)
{
  try {
    Napi::String logLevel = info[0].As<Napi::String>();
    Napi::String message = info[1].As<Napi::String>();

    auto messageStr = static_cast<std::string>(message);
    while (!messageStr.empty() && messageStr.back() == '\n') {
      messageStr.pop_back();
    }

    if (static_cast<std::string>(logLevel) == "info") {
      GetLogger()->info(messageStr);
    } else if (static_cast<std::string>(logLevel) == "error") {
      GetLogger()->error(messageStr);
    }
  } catch (std::exception& e) {
    // No sense to rethrow, NodeJS will unlikely be able to print this
    // exception
    GetLogger()->error("ScampServer::WriteLogs - {}", e.what());
  }
  return info.Env().Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  return ScampServer::Init(env, exports);
}

NODE_API_MODULE(scamp, Init)
