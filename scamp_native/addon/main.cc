#include "AsyncSaveStorage.h"
#include "FormCallbacks.h"
#include "GamemodeApi.h"
#include "MigrationDatabase.h"
#include "MongoDatabase.h"
#include "Networking.h"
#include "NetworkingCombined.h"
#include "NetworkingMock.h"
#include "PartOne.h"
#include "ScriptStorage.h"
#include "SqliteDatabase.h"
#include <cassert>
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
  Napi::Value GetMpApi(const Napi::CallbackInfo& info);

private:
  std::shared_ptr<PartOne> partOne;
  std::shared_ptr<Networking::IServer> server;
  std::shared_ptr<Networking::MockServer> serverMock;
  std::shared_ptr<ScampServerListener> listener;
  Napi::Env tickEnv;
  Napi::ObjectReference emitter;
  Napi::FunctionReference emit;
  std::optional<Napi::ObjectReference> mp;
  std::shared_ptr<spdlog::logger> logger;
  nlohmann::json serverSettings;

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
    if (server.mp == std::nullopt)
      return true;

    auto f = server.mp->Get(eventName);
    if (!f.IsFunction())
      return true;

    auto& env = server.tickEnv;

    if (args && !args->is_array())
      return true;

    std::vector<Napi::Value> argumentsInNapiFormat;
    if (formId != std::nullopt) {
      argumentsInNapiFormat.push_back(Napi::Number::New(env, *formId));
    }

    if (args) {
      auto& argsArray = args->get_array().value();
      for (size_t i = 0; i < argsArray.size(); ++i) {
        std::string elementString = simdjson::minify(argsArray.at(i));
        auto builtinJson = env.Global().Get("JSON").As<Napi::Object>();
        auto parse = builtinJson.Get("parse").As<Napi::Function>();
        Napi::Value resultOfParsing =
          parse.Call(builtinJson, { Napi::String::New(env, elementString) });
        argumentsInNapiFormat.push_back(resultOfParsing);
      }
    }

    std::vector<napi_value> argumentsInNodeFormat;
    argumentsInNodeFormat.reserve(argumentsInNapiFormat.size());
    for (auto& arg : argumentsInNapiFormat) {
      argumentsInNodeFormat.push_back(arg);
    }

    try {
      auto callResult = f.As<Napi::Function>().Call(
        env.Undefined(), argumentsInNodeFormat.size(),
        argumentsInNodeFormat.data());

      if (callResult.IsUndefined())
        return true;

      return static_cast<bool>(callResult.ToBoolean());
    } catch (Napi::Error& e) {
      std::cout << "[" << eventName << "] "
                << " " << e.Message() << std::endl;
      return true;
    }
  }

private:
  ScampServer& server;
};

Napi::FunctionReference ScampServer::constructor;

Napi::Object ScampServer::Init(Napi::Env env, Napi::Object exports)
{
  Napi::Function func = DefineClass(
    env, "ScampServer",
    { InstanceMethod<&ScampServer::AttachSaveStorage>("attachSaveStorage"),
      InstanceMethod<&ScampServer::Tick>("tick"),
      InstanceMethod<&ScampServer::On>("on"),
      InstanceMethod<&ScampServer::CreateActor>("createActor"),
      InstanceMethod<&ScampServer::SetUserActor>("setUserActor"),
      InstanceMethod<&ScampServer::GetUserActor>("getUserActor"),
      InstanceMethod<&ScampServer::GetActorPos>("getActorPos"),
      InstanceMethod<&ScampServer::GetActorCellOrWorld>("getActorCellOrWorld"),
      InstanceMethod<&ScampServer::GetActorName>("getActorName"),
      InstanceMethod<&ScampServer::DestroyActor>("destroyActor"),
      InstanceMethod<&ScampServer::SetRaceMenuOpen>("setRaceMenuOpen"),
      InstanceMethod<&ScampServer::SendCustomPacket>("sendCustomPacket"),
      InstanceMethod<&ScampServer::GetActorsByProfileId>(
        "getActorsByProfileId"),
      InstanceMethod<&ScampServer::SetEnabled>("setEnabled"),
      InstanceMethod<&ScampServer::CreateBot>("createBot"),
      InstanceMethod<&ScampServer::GetUserByActor>("getUserByActor"),
      InstanceMethod<&ScampServer::GetMpApi>("getMpApi") });
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
    : std::string("sqlite");

  if (databaseDriver == "sqlite") {
    auto databaseName = settings.count("databaseName")
      ? settings["databaseName"].get<std::string>()
      : std::string("world.sqlite");

    logger->info("Using sqlite with name '" + databaseName + "'");
    return std::make_shared<SqliteDatabase>(databaseName);
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

}

ScampServer::ScampServer(const Napi::CallbackInfo& info)
  : ObjectWrap(info)
  , tickEnv(info.Env())
{
  try {
    partOne.reset(new PartOne);
    partOne->EnableProductionHacks();
    listener.reset(new ScampServerListener(*this));
    partOne->AddListener(listener);
    Napi::Number port = info[0].As<Napi::Number>(),
                 maxConnections = info[1].As<Napi::Number>();

    serverMock = std::make_shared<Networking::MockServer>();

    std::string dataDir =
      "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special "
      "Edition\\Data";
#ifndef WIN32
    dataDir = "/skyrim_data_dir";
#endif

    auto logger = spdlog::stdout_color_mt("console");
    partOne->AttachLogger(logger);

    std::ifstream f("server-settings.json");
    std::stringstream buffer;
    buffer << f.rdbuf();
    auto serverSettings = nlohmann::json::parse(buffer.str());
    if (serverSettings["dataDir"] != nullptr) {
      dataDir = serverSettings["dataDir"];
    }
    logger->info("Using data dir '{}'", dataDir);

    std::vector<espm::fs::path> plugins = { "Skyrim.esm", "Update.esm",
                                            "Dawnguard.esm", "HearthFires.esm",
                                            "Dragonborn.esm" };
    if (serverSettings["loadOrder"].is_array()) {
      plugins.clear();
      for (size_t i = 0; i < serverSettings["loadOrder"].size(); ++i) {
        auto s = static_cast<std::string>(serverSettings["loadOrder"][i]);
        plugins.push_back(s);
      }
    }

    auto scriptStorage = std::make_shared<DirectoryScriptStorage>(
      (espm::fs::path(dataDir) / "scripts").string());

    auto espm = new espm::Loader(dataDir, plugins);
    auto realServer = Networking::CreateServer(
      static_cast<uint32_t>(port), static_cast<uint32_t>(maxConnections));
    server = Networking::CreateCombinedServer({ realServer, serverMock });
    partOne->SetSendTarget(server.get());
    partOne->worldState.AttachScriptStorage(scriptStorage);
    partOne->AttachEspm(espm);
    this->serverSettings = serverSettings;
    this->logger = logger;

    auto reloot = serverSettings["reloot"];
    for (auto it = reloot.begin(); it != reloot.end(); ++it) {
      std::string recordType = it.key();
      auto timeMs = static_cast<int>(it.value());
      auto time = std::chrono::milliseconds(1) * timeMs;
      partOne->worldState.SetRelootTime(recordType, time);
      logger->info("'{}' will be relooted every {} ms", recordType, timeMs);
    }

    auto res =
      info.Env().RunScript("let require = global.require || "
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

uint32_t GetFormId(Napi::Value v)
{
  if (v.IsNumber()) {
    double formId = static_cast<double>(v.As<Napi::Number>());
    constexpr auto max =
      static_cast<double>(std::numeric_limits<uint32_t>::max());
    if (std::isfinite(formId) && formId >= 0 && formId < max) {
      return static_cast<uint32_t>(floor(formId));
    }
  }
  return 0;
}

std::string ExtractString(Napi::Value v, const char* argName)
{
  if (!v.IsString()) {
    std::stringstream ss;
    ss << "Expected '" << argName << "' to be string, but got '";
    ss << static_cast<std::string>(v.ToString().As<Napi::String>());
    ss << "'";
    Err(v.Env(), ss.str());
  }
  return static_cast<std::string>(v.As<Napi::String>());
}

std::string ExtractPropertyName(Napi::Value v)
{
  return ExtractString(v, "propertyName");
}

uint32_t ExtractFormId(Napi::Value v, const char* argName = "formId")
{
  if (!v.IsNumber()) {
    std::stringstream ss;
    ss << "Expected '" << argName << "' to be number, but got '";
    ss << static_cast<std::string>(v.ToString().As<Napi::String>());
    ss << "'";
    Err(v.Env(), ss.str());
  }
  return GetFormId(v);
}

nlohmann::json ExtractNewValue(Napi::Value v)
{
  auto builtinJson = v.Env().Global().Get("JSON").As<Napi::Object>();
  auto stringify = builtinJson.Get("stringify").As<Napi::Function>();
  std::string dump = stringify.Call(builtinJson, { v }).As<Napi::String>();
  return nlohmann::json::parse(dump);
}

void EnsurePropertyExists(Napi::Env env,
                          const std::shared_ptr<GamemodeApi::State>& state,
                          const std::string& propertyName)
{
  if (!state->createdProperties.count(propertyName)) {
    std::stringstream ss;
    ss << "Property '" << propertyName << "' doesn't exist";
    Err(env, ss.str());
  }
}

Napi::Value ParseJson(const Napi::CallbackInfo& info, const std::string& dump)
{
  auto builtinJson = info.Env().Global().Get("JSON").As<Napi::Object>();
  auto parse = builtinJson.Get("parse").As<Napi::Function>();
  return parse.Call({ Napi::String::New(info.Env(), dump) });
}

Napi::Value ScampServer::GetMpApi(const Napi::CallbackInfo& info)
{
  Napi::Object mp = Napi::Object::New(info.Env());
  if (this->mp != std::nullopt) {
    return this->mp->Value();
  }
  this->mp = Napi::Persistent(mp);

  auto state = std::make_shared<GamemodeApi::State>();

  auto update = [this, state] {
    partOne->NotifyGamemodeApiStateChanged(*state);
  };

  mp.Set("clear",
         Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
           try {
             *state = GamemodeApi::State();
             update();
           } catch (std::exception& e) {
             throw Napi::Error::New(info.Env(), (std::string)e.what());
           }
         }));

  mp.Set(
    "makeProperty",
    Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
      try {
        auto propertyName = ExtractPropertyName(info[0]);
        if (propertyName.size() < 1 || propertyName.size() > 128) {
          std::stringstream ss;
          ss << "The length of 'propertyName' must be between 1 and 128, but "
                "it "
                "is '";
          ss << propertyName.size();
          ss << "'";
          Err(info, ss.str());
        }

        auto alphabet = GetPropertyAlphabet();
        if (propertyName.find_first_not_of(alphabet.data()) !=
            std::string::npos) {
          std::stringstream ss;
          ss << "'propertyName' may contain only Latin characters, numbers, "
                "and underscore";
          Err(info, ss.str());
        }

        if (state->createdProperties.count(propertyName)) {
          std::stringstream ss;
          ss << "'propertyName' must be unique";
          Err(info, ss.str());
        }

        GamemodeApi::PropertyInfo propertyInfo;

        if (!info[1].IsObject()) {
          std::stringstream ss;
          ss << "Expected 'options' to be object, but got '";
          ss << static_cast<std::string>(
            info[1].ToString().As<Napi::String>());
          ss << "'";
          Err(info, ss.str());
        }

        auto options = info[1].As<Napi::Object>();

        std::vector<std::pair<std::string, bool*>> booleans{
          { "isVisibleByOwner", &propertyInfo.isVisibleByOwner },
          { "isVisibleByNeighbors", &propertyInfo.isVisibleByNeighbors }
        };
        for (auto [optionName, ptr] : booleans) {
          auto v = options.Get(optionName.data());
          if (!v.IsBoolean()) {
            std::stringstream ss;
            ss << "Expected 'options." << optionName;
            ss << "' to be boolean, but got '";
            ss << static_cast<std::string>(v.ToString().As<Napi::String>());
            ss << "'";
            Err(info, ss.str());
          }
          *ptr = static_cast<bool>(v.As<Napi::Boolean>());
        }

        std::vector<std::pair<std::string, std::string*>> strings{
          { "updateNeighbor", &propertyInfo.updateNeighbor },
          { "updateOwner", &propertyInfo.updateOwner }
        };
        for (auto [optionName, ptr] : strings) {
          auto v = options.Get(optionName.data());
          if (!v.IsString()) {
            std::stringstream ss;
            ss << "Expected 'options." << optionName;
            ss << "' to be string, but got '";
            ss << static_cast<std::string>(v.ToString().As<Napi::String>());
            ss << "'";
            Err(info, ss.str());
          }
          *ptr = static_cast<std::string>(v.As<Napi::String>());
        }

        state->createdProperties[propertyName] = propertyInfo;

        update();
      } catch (std::exception& e) {
        throw Napi::Error::New(info.Env(), (std::string)e.what());
      }
    }));

  mp.Set("makeEventSource",
         Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
           try {

             if (!info[0].IsString()) {
               std::stringstream ss;
               ss << "Expected 'eventName' to be string, but got '";
               ss << static_cast<std::string>(
                 info[0].ToString().As<Napi::String>());
               ss << "'";
               Err(info, ss.str());
             }

             auto eventName =
               static_cast<std::string>(info[0].As<Napi::String>());

             if (state->createdEventSources.count(eventName)) {
               std::stringstream ss;
               ss << "Expected 'eventName' to be string, but got '";
               ss << static_cast<std::string>(
                 info[0].ToString().As<Napi::String>());
               ss << "'";
               Err(info, ss.str());
             }

             if (!info[1].IsString()) {
               std::stringstream ss;
               ss << "Expected 'functionBody' to be string, but got '";
               ss << static_cast<std::string>(
                 info[1].ToString().As<Napi::String>());
               ss << "'";
               Err(info, ss.str());
             }

             auto functionBody =
               static_cast<std::string>(info[1].As<Napi::String>());
             state->createdEventSources[eventName] = { functionBody };

             update();
           } catch (std::exception& e) {
             throw Napi::Error::New(info.Env(), (std::string)e.what());
           }
         }));

  mp.Set("get",
         Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
           try {
             auto propertyName = ExtractPropertyName(info[1]);
             auto formId = ExtractFormId(info[0]);

             auto& refr =
               partOne->worldState.GetFormAt<MpObjectReference>(formId);

             Napi::Value res = info.Env().Undefined();

             if (propertyName == "type") {
               if (dynamic_cast<MpActor*>(&refr)) {
                 res = Napi::String::New(info.Env(), "MpActor");
               } else {
                 res = Napi::String::New(info.Env(), "MpObjectReference");
               }
             } else if (propertyName == "pos" || propertyName == "angle") {
               auto niPoint3 =
                 propertyName == "pos" ? refr.GetPos() : refr.GetAngle();
               auto arr = Napi::Array::New(info.Env(), 3);
               for (uint32_t i = 0; i < 3; ++i) {
                 arr.Set(i, Napi::Number::New(info.Env(), niPoint3[i]));
               }
               res = arr;
             } else if (propertyName == "worldOrCellDesc") {
               auto desc = FormDesc::FromFormId(refr.GetCellOrWorld(),
                                                partOne->worldState.espmFiles);
               res = Napi::String::New(info.Env(), desc.ToString());
             } else if (propertyName == "baseDesc") {
               auto desc = FormDesc::FromFormId(refr.GetBaseId(),
                                                partOne->worldState.espmFiles);
               res = Napi::String::New(info.Env(), desc.ToString());
             } else if (propertyName == "isOpen") {
               res = Napi::Boolean::New(info.Env(), refr.IsOpen());
             } else if (propertyName == "appearance") {
               if (auto actor = dynamic_cast<MpActor*>(&refr)) {
                 auto& dump = actor->GetLookAsJson();
                 if (dump.size() > 0) {
                   res = ParseJson(info, dump);
                 }
               }
             } else if (propertyName == "inventory") {
               res = ParseJson(info, refr.GetInventory().ToJson().dump());
             } else if (propertyName == "equipment") {
               if (auto actor = dynamic_cast<MpActor*>(&refr)) {
                 auto& dump = actor->GetEquipmentAsJson();
                 if (dump.size() > 0) {
                   res = ParseJson(info, dump);
                 }
               }
             } else if (propertyName == "isOnline") {
               res = Napi::Boolean::New(info.Env(), false);
               if (auto actor = dynamic_cast<MpActor*>(&refr)) {
                 auto userId = partOne->serverState.UserByActor(actor);
                 if (userId != Networking::InvalidUserId) {
                   res = Napi::Boolean::New(info.Env(), true);
                 }
               }
             } else if (propertyName == "formDesc") {
               auto desc = FormDesc::FromFormId(refr.GetFormId(),
                                                partOne->worldState.espmFiles);
               res = Napi::String::New(info.Env(), desc.ToString());
             } else if (propertyName == "neighbors") {
               std::set<uint32_t> ids;
               for (auto listener : refr.GetListeners()) {
                 ids.insert(listener->GetFormId());
               }
               for (auto emitter : refr.GetEmitters()) {
                 ids.insert(emitter->GetFormId());
               }
               auto arr = Napi::Array::New(info.Env(), ids.size());
               size_t i = 0;
               for (auto id : ids) {
                 arr.Set(i, id);
                 ++i;
               }
               res = arr;
             } else if (propertyName == "isDisabled") {
               res = Napi::Boolean::New(info.Env(), refr.IsDisabled());
             } else {
               EnsurePropertyExists(info.Env(), state, propertyName);

               auto fields = refr.GetChangeForm().dynamicFields;

               auto it = fields.find(propertyName);
               if (it != fields.end()) {
                 auto dump = it->dump();
                 res = ParseJson(info, dump);
               }
             }

             return res;

           } catch (std::exception& e) {
             throw Napi::Error::New(info.Env(), (std::string)e.what());
           }
         }));

  mp.Set(
    "set",
    Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
      try {
        auto formId = ExtractFormId(info[0]);
        auto propertyName = ExtractPropertyName(info[1]);
        auto newValue = ExtractNewValue(info[2]);

        auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

        if (propertyName == "pos") {
          float x = newValue[0].get<float>();
          float y = newValue[1].get<float>();
          float z = newValue[2].get<float>();
          refr.SetPos({ x, y, z });
          refr.SetTeleportFlag(true);
        } else if (propertyName == "angle") {
          float x = newValue[0].get<float>();
          float y = newValue[1].get<float>();
          float z = newValue[2].get<float>();
          refr.SetAngle({ x, y, z });
          refr.SetTeleportFlag(true);
        } else if (propertyName == "worldOrCellDesc") {
          std::string str = newValue.get<std::string>();
          uint32_t formId =
            FormDesc::FromString(str).ToFormId(partOne->worldState.espmFiles);
          refr.SetCellOrWorld(formId);
        } else if (propertyName == "isOpen") {
          refr.SetOpen(newValue.get<bool>());
        } else if (propertyName == "appearance") {
          if (auto actor = dynamic_cast<MpActor*>(&refr)) {
            // TODO: Live update of look
            if (newValue.is_object()) {
              auto look = Look::FromJson(newValue);
              actor->SetLook(&look);
            } else {
              actor->SetLook(nullptr);
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
          throw std::runtime_error(
            "mp.set is not implemented for 'equipment'");
        } else if (propertyName == "isOnline") {
          throw std::runtime_error("mp.set is not implemented for 'isOnline'");
        } else if (propertyName == "formDesc") {
          throw std::runtime_error("mp.set is not implemented for 'formDesc'");
        } else if (propertyName == "neighbors") {
          throw std::runtime_error(
            "mp.set is not implemented for 'neighbors'");
        } else if (propertyName == "isDisabled") {
          if (refr.GetFormId() < 0xff000000)
            throw std::runtime_error(
              "'isDisabled' is not usable for non-FF forms");
          newValue.get<bool>() ? refr.Disable() : refr.Enable();
        } else {

          EnsurePropertyExists(info.Env(), state, propertyName);

          auto& info = state->createdProperties[propertyName];

          refr.SetProperty(propertyName, newValue, info.isVisibleByOwner,
                           info.isVisibleByNeighbors);
        }

      } catch (std::exception& e) {
        throw Napi::Error::New(info.Env(), (std::string)e.what());
      }
    }));

  mp.Set(
    "place",
    Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
      try {
        auto globalRecordId = ExtractFormId(info[0], "globalRecordId");
        auto akFormToPlace =
          partOne->GetEspm().GetBrowser().LookupById(globalRecordId);
        if (!akFormToPlace.rec) {
          std::stringstream ss;
          ss << std::hex << "Bad record Id " << globalRecordId;
          throw std::runtime_error(ss.str());
        }

        std::string type = akFormToPlace.rec->GetType().ToString();

        LocationalData locationalData = { { 0, 0, 0 }, { 0, 0, 0 }, 0x3c };
        FormCallbacks callbacks = partOne->CreateFormCallbacks();

        std::unique_ptr<MpObjectReference> newRefr;
        if (akFormToPlace.rec->GetType() == "NPC_") {
          auto actor = new MpActor(locationalData, callbacks, globalRecordId);
          newRefr.reset(actor);
        } else
          newRefr.reset(new MpObjectReference(locationalData, callbacks,
                                              globalRecordId, type));

        auto worldState = &partOne->worldState;
        auto newRefrId = worldState->GenerateFormId();
        worldState->AddForm(std::move(newRefr), newRefrId);

        auto& refr = worldState->GetFormAt<MpObjectReference>(newRefrId);
        refr.ForceSubscriptionsUpdate();

        return Napi::Number::New(info.Env(), refr.GetFormId());
      } catch (std::exception& e) {
        throw Napi::Error::New(info.Env(), (std::string)e.what());
      }
    }));

  mp.Set(
    "lookupEspmRecordById",
    Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
      try {
        auto globalRecordId = ExtractFormId(info[0], "globalRecordId");

        auto espmLookupResult = Napi::Object::New(info.Env());

        auto lookupRes =
          partOne->GetEspm().GetBrowser().LookupById(globalRecordId);
        if (lookupRes.rec) {
          auto fields = Napi::Array::New(info.Env(), 0);

          espm::IterateFields_(
            lookupRes.rec,
            [&](const char* type, uint32_t size, const char* data) {
              auto uint8arr = Napi::Uint8Array::New(info.Env(), size);
              memcpy(uint8arr.Data(), data, size);

              auto push = fields.Get("push").As<Napi::Function>();
              auto field = Napi::Object::New(info.Env());
              field.Set("type",
                        Napi::String::New(info.Env(), std::string(type, 4)));
              field.Set("data", uint8arr);
              push.Call(fields, { field });
            },
            &partOne->worldState.GetEspmCache());

          auto id = Napi::Number::New(info.Env(), lookupRes.rec->GetId());
          auto edid =
            Napi::String::New(info.Env(), lookupRes.rec->GetEditorId());
          auto type =
            Napi::String::New(info.Env(), lookupRes.rec->GetType().ToString());
          auto flags =
            Napi::Number::New(info.Env(), lookupRes.rec->GetFlags());

          auto record = Napi::Object::New(info.Env());
          record.Set("id", id);
          record.Set("editorId", edid);
          record.Set("type", type);
          record.Set("flags", flags);
          record.Set("fields", fields);
          espmLookupResult.Set("record", record);

          espmLookupResult.Set(
            "fileIndex", Napi::Number::New(info.Env(), lookupRes.fileIdx));
        }

        return espmLookupResult;

      } catch (std::exception& e) {
        throw Napi::Error::New(info.Env(), (std::string)e.what());
      }
    }));

  mp.Set("getEspmLoadOrder",
         Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
           try {
             auto fileNames = partOne->GetEspm().GetFileNames();
             auto arr = Napi::Array::New(info.Env(), fileNames.size());
             for (uint32_t i = 0; i < static_cast<uint32_t>(fileNames.size());
                  ++i) {
               arr[i] = fileNames[i];
             }
             return arr;
           } catch (std::exception& e) {
             throw Napi::Error::New(info.Env(), (std::string)e.what());
           }
         }));

  mp.Set("getDescFromId",
         Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
           try {
             auto formId = ExtractFormId(info[0]);
             auto espmFileNames = partOne->GetEspm().GetFileNames();
             auto formDesc = FormDesc::FromFormId(formId, espmFileNames);

             return Napi::String::New(info.Env(), formDesc.ToString());
           } catch (std::exception& e) {
             throw Napi::Error::New(info.Env(), (std::string)e.what());
           }
         }));

  mp.Set("getIdFromDesc",
         Napi::Function::New(info.Env(), [=](const Napi::CallbackInfo& info) {
           try {
             auto str = ExtractString(info[0], "formDesc");
             auto espmFileNames = partOne->GetEspm().GetFileNames();
             auto formDesc = FormDesc::FromString(str);

             return Napi::Number::New(info.Env(),
                                      formDesc.ToFormId(espmFileNames));
           } catch (std::exception& e) {
             throw Napi::Error::New(info.Env(), (std::string)e.what());
           }
         }));

  return mp;
}

Napi::String Method(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();
  return Napi::String::New(env, "world");
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  return ScampServer::Init(env, exports);
}

NODE_API_MODULE(scamp, Init)
