#include "ScampServer.h"

#include "AsyncSaveStorage.h"
#include "Bot.h"
#include "EspmGameObject.h"
#include "FileDatabase.h"
#include "FormCallbacks.h"
#include "GamemodeApi.h"
#include "MigrationDatabase.h"
#include "MongoDatabase.h"
#include "MpFormGameObject.h"
#include "NapiHelper.h"
#include "NetworkingCombined.h"
#include "PacketHistoryWrapper.h"
#include "PapyrusUtils.h"
#include "ScampServerListener.h"
#include "ScriptStorage.h"
#include "SettingsUtils.h"
#include "formulas/SweetPieDamageFormula.h"
#include "formulas/TES5DamageFormula.h"
#include "property_bindings/PropertyBindingFactory.h"
#include <cassert>
#include <cctype>
#include <memory>
#include <napi.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace {
constexpr size_t kMockServerIdx = 1;

std::shared_ptr<spdlog::logger>& GetLogger()
{
  static auto g_logger = spdlog::stdout_color_mt("console");
  return g_logger;
}

std::shared_ptr<ISaveStorage> CreateSaveStorage(
  std::shared_ptr<IDatabase> db, std::shared_ptr<spdlog::logger> logger)
{
  return std::make_shared<AsyncSaveStorage>(db, logger);
}

std::string GetPropertyAlphabet()
{
  std::string alphabet;
  for (char c = 'a'; c <= 'z'; c++) {
    alphabet += c;
  }
  for (char c = 'A'; c <= 'Z'; c++) {
    alphabet += c;
  }
  for (char c = '0'; c <= '9'; c++) {
    alphabet += c;
  }
  alphabet += '_';
  return alphabet;
}
}

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
      InstanceMethod("getActorsByProfileId",
                     &ScampServer::GetActorsByProfileId),
      InstanceMethod("setEnabled", &ScampServer::SetEnabled),
      InstanceMethod("createBot", &ScampServer::CreateBot),
      InstanceMethod("getUserByActor", &ScampServer::GetUserByActor),
      InstanceMethod("writeLogs", &ScampServer::WriteLogs),

      InstanceMethod("getLocalizedString", &ScampServer::GetLocalizedString),
      InstanceMethod("getServerSettings", &ScampServer::GetServerSettings),
      InstanceMethod("clear", &ScampServer::Clear),
      InstanceMethod("makeProperty", &ScampServer::MakeProperty),
      InstanceMethod("makeEventSource", &ScampServer::MakeEventSource),
      InstanceMethod("get", &ScampServer::Get),
      InstanceMethod("set", &ScampServer::Set),
      InstanceMethod("place", &ScampServer::Place),
      InstanceMethod("lookupEspmRecordById",
                     &ScampServer::LookupEspmRecordById),
      InstanceMethod("getEspmLoadOrder", &ScampServer::GetEspmLoadOrder),
      InstanceMethod("getDescFromId", &ScampServer::GetDescFromId),
      InstanceMethod("getIdFromDesc", &ScampServer::GetIdFromDesc),
      InstanceMethod("callPapyrusFunction", &ScampServer::CallPapyrusFunction),
      InstanceMethod("registerPapyrusFunction",
                     &ScampServer::RegisterPapyrusFunction),
      InstanceMethod("sendCustomPacket", &ScampServer::SendCustomPacket),
      InstanceMethod("setPacketHistoryRecording",
                     &ScampServer::SetPacketHistoryRecording),
      InstanceMethod("getPacketHistory", &ScampServer::GetPacketHistory),
      InstanceMethod("clearPacketHistory", &ScampServer::ClearPacketHistory),
      InstanceMethod("requestPacketHistoryPlayback",
                     &ScampServer::RequestPacketHistoryPlayback) });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("ScampServer", func);
  return exports;
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

    auto espm = new espm::Loader(pluginPaths);
    std::string password = serverSettings.contains("password")
      ? static_cast<std::string>(serverSettings["password"])
      : std::string(kNetworkingPassword);
    auto realServer = Networking::CreateServer(
      static_cast<uint32_t>(port), static_cast<uint32_t>(maxConnections),
      password.data());

    static_assert(kMockServerIdx == 1);
    server = Networking::CreateCombinedServer({ realServer, serverMock });

    partOne->SetSendTarget(server.get());

    const auto& sweetPieDamageFormulaSettings =
      serverSettings["sweetPieDamageFormulaSettings"];
    if (sweetPieDamageFormulaSettings.is_object()) {
      partOne->SetDamageFormula(std::make_unique<SweetPieDamageFormula>(
        std::make_unique<TES5DamageFormula>(), sweetPieDamageFormulaSettings));
    } else {
      partOne->SetDamageFormula(std::make_unique<TES5DamageFormula>());
    }

    std::vector<std::shared_ptr<IScriptStorage>> scriptStorages;
    scriptStorages.push_back(std::make_shared<DirectoryScriptStorage>(
      (espm::fs::path(dataDir) / "scripts").string()));
    scriptStorages.push_back(std::make_shared<AssetsScriptStorage>());
    auto scriptStorage =
      std::make_shared<CombinedScriptStorage>(scriptStorages);
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

    auto res =
      NapiHelper::RunScript(Env(),
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
    partOne->AttachSaveStorage(CreateSaveStorage(
      SettingsUtils::CreateDatabase(serverSettings, logger), logger));
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::Tick(const Napi::CallbackInfo& info)
{
  try {
    tickEnv = info.Env();

    bool tickFinished = false;
    while (!tickFinished) {
      try {
        server->Tick(PartOne::HandlePacket, partOne.get());
        tickFinished = true;
      } catch (std::exception& e) {
        logger->error("{}", e.what());
      }
    }

    partOne->Tick();
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
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
  auto formId = NapiHelper::ExtractUInt32(info[0], "formId");
  auto pos = NapiHelper::ExtractNiPoint3(info[1], "pos");
  auto angleZ = NapiHelper::ExtractFloat(info[2], "angleZ");
  auto cellOrWorld = NapiHelper::ExtractUInt32(info[3], "cellOrWorld");

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
  if (!this->serverMock) {
    throw Napi::Error::New(info.Env(), "Bad serverMock");
  }
  auto serverCombined =
    std::dynamic_pointer_cast<Networking::ServerCombined>(this->server);
  if (!serverCombined) {
    throw Napi::Error::New(
      info.Env(),
      "Expected server to be instance of Networking::ServerCombined");
  }

  auto pair = this->serverMock->CreateClient();
  auto bot = std::make_shared<Bot>(pair.first);

  auto jBot = Napi::Object::New(info.Env());

  jBot.Set(
    "getUserId",
    Napi::Function::New(
      info.Env(), [bot, pair, serverCombined](const Napi::CallbackInfo& info) {
        try {
          Networking::UserId realUserId = pair.second;
          return Napi::Number::New(
            info.Env(),
            serverCombined->GetCombinedUserId(kMockServerIdx, realUserId));
        } catch (std::exception& e) {
          throw Napi::Error::New(info.Env(), std::string(e.what()));
        }
      }));

  jBot.Set(
    "destroy",
    Napi::Function::New(info.Env(), [bot](const Napi::CallbackInfo& info) {
      try {
        bot->Destroy();
        return info.Env().Undefined();
      } catch (std::exception& e) {
        throw Napi::Error::New(info.Env(), std::string(e.what()));
      }
    }));
  jBot.Set(
    "send",
    Napi::Function::New(info.Env(), [bot](const Napi::CallbackInfo& info) {
      try {
        auto argument = info[0];
        if (argument.IsTypedArray()) {
          auto arr = argument.As<Napi::Uint8Array>();
          size_t n = arr.ByteLength();
          char* data = reinterpret_cast<char*>(arr.Data());
          std::string s(data, n);
          bot->Send(s);
        } else {
          auto standardJson =
            info.Env().Global().Get("JSON").As<Napi::Object>();
          auto stringify = standardJson.Get("stringify").As<Napi::Function>();
          std::string s;
          s += Networking::MinPacketId;
          s += static_cast<std::string>(
            stringify.Call({ info[0] }).As<Napi::String>());
          bot->Send(s);
        }

        // Memory leak fix
        // TODO: Provide tick API
        bot->Tick();

        return info.Env().Undefined();
      } catch (std::exception& e) {
        throw Napi::Error::New(info.Env(), std::string(e.what()));
      }
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

Napi::Value ScampServer::GetLocalizedString(const Napi::CallbackInfo& info)
{
  try {
    auto translatedString = info.Env().Undefined();

    if (!localizationProvider) {
      return translatedString;
    }

    auto globalRecordId = NapiHelper::ExtractUInt32(info[0], "globalRecordId");
    auto lookupRes =
      partOne->GetEspm().GetBrowser().LookupById(globalRecordId);

    if (!lookupRes.rec) {
      return translatedString;
    }

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

          std::transform(fileNameWithoutExt.begin(), fileNameWithoutExt.end(),
                         fileNameWithoutExt.begin(),
                         [](unsigned char c) { return std::tolower(c); });

          translatedString = Napi::String::New(
            info.Env(),
            this->localizationProvider->Get(fileNameWithoutExt, stringId));
        }
      },
      cache);

    return translatedString;
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::GetServerSettings(const Napi::CallbackInfo& info)
{
  try {
    return NapiHelper::ParseJson(info.Env(), serverSettings);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
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

Napi::Value ScampServer::MakeProperty(const Napi::CallbackInfo& info)
{
  try {
    static const auto kAlphabet = GetPropertyAlphabet();
    static const std::pair<size_t, size_t> kMinMaxSize = { 1, 128 };
    auto isUnique = [this](const std::string& s) {
      return gamemodeApiState.createdProperties.count(s) == 0;
    };
    auto propertyName = NapiHelper::ExtractString(
      info[0], "propertyName", kAlphabet, kMinMaxSize, isUnique);

    GamemodeApi::PropertyInfo propertyInfo;

    auto options = NapiHelper::ExtractObject(info[1], "options");

    std::vector<std::pair<std::string, bool*>> booleans{
      { "isVisibleByOwner", &propertyInfo.isVisibleByOwner },
      { "isVisibleByNeighbors", &propertyInfo.isVisibleByNeighbors }
    };
    for (auto [optionName, ptr] : booleans) {
      std::string argName = "options." + optionName;
      auto v = NapiHelper::ExtractBoolean(options.Get(optionName.data()),
                                          argName.data());
      *ptr = static_cast<bool>(v);
    }

    std::vector<std::pair<std::string, std::string*>> strings{
      { "updateNeighbor", &propertyInfo.updateNeighbor },
      { "updateOwner", &propertyInfo.updateOwner }
    };
    for (auto [optionName, ptr] : strings) {
      std::string argName = "options." + optionName;
      static const std::pair<size_t, size_t> kMinMaxSize = { 0, 100 * 1024 };
      auto v =
        NapiHelper::ExtractString(options.Get(optionName.data()),
                                  argName.data(), std::nullopt, kMinMaxSize);
      *ptr = static_cast<std::string>(v);
    }

    gamemodeApiState.createdProperties[propertyName] = propertyInfo;
    partOne->NotifyGamemodeApiStateChanged(gamemodeApiState);

    return info.Env().Undefined();
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::MakeEventSource(const Napi::CallbackInfo& info)
{
  try {
    static const auto kAlphabet = GetPropertyAlphabet();
    static const std::pair<size_t, size_t> kMinMaxSizeEventName = { 1, 128 };
    auto isUnique = [this](const std::string& s) {
      return gamemodeApiState.createdEventSources.count(s) == 0;
    };
    auto eventName = NapiHelper::ExtractString(info[0], "eventName", kAlphabet,
                                               kMinMaxSizeEventName);

    static const std::pair<size_t, size_t> kMinMaxSizeFunctionBody = {
      0, 100 * 1024
    };
    auto functionBody = NapiHelper::ExtractString(
      info[1], "functionBody", std::nullopt, kMinMaxSizeFunctionBody);
    gamemodeApiState.createdEventSources[eventName] = { functionBody };
    partOne->NotifyGamemodeApiStateChanged(gamemodeApiState);
    return info.Env().Undefined();
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::Get(const Napi::CallbackInfo& info)
{
  try {
    auto formId = NapiHelper::ExtractUInt32(info[0], "formId");
    auto propertyName = NapiHelper::ExtractString(info[1], "propertyName");

    static auto g_standardPropertyBindings =
      PropertyBindingFactory().CreateStandardPropertyBindings();

    auto it = g_standardPropertyBindings.find(propertyName);
    if (it != g_standardPropertyBindings.end()) {
      auto res = it->second->Get(info.Env(), *this, formId);
      if (spdlog::should_log(spdlog::level::trace)) {
        spdlog::trace("ScampServer::Get {:x} - {}={} (native property)",
                      formId, propertyName,
                      static_cast<std::string>(res.ToString()));
      }
      return res;
    } else {
      auto res = PropertyBindingFactory()
                   .CreateCustomPropertyBinding(propertyName)
                   ->Get(info.Env(), *this, formId);
      if (spdlog::should_log(spdlog::level::trace)) {
        spdlog::trace("ScampServer::Get {:x} - {}={} (custom property)",
                      formId, propertyName,
                      static_cast<std::string>(res.ToString()));
      }
      return res;
    }

  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::Set(const Napi::CallbackInfo& info)
{
  try {
    auto formId = NapiHelper::ExtractUInt32(info[0], "formId");
    auto propertyName = NapiHelper::ExtractString(info[1], "propertyName");
    auto value = info[2];

    static auto g_standardPropertyBindings =
      PropertyBindingFactory().CreateStandardPropertyBindings();

    auto it = g_standardPropertyBindings.find(propertyName);
    if (it != g_standardPropertyBindings.end()) {
      if (spdlog::should_log(spdlog::level::trace)) {
        spdlog::trace("ScampServer::Set {:x} - {}={} (native property)",
                      formId, propertyName,
                      static_cast<std::string>(value.ToString()));
      }
      it->second->Set(info.Env(), *this, formId, value);
    } else {
      if (spdlog::should_log(spdlog::level::trace)) {
        spdlog::trace("ScampServer::Set {:x} - {}={} (custom property)",
                      formId, propertyName,
                      static_cast<std::string>(value.ToString()));
      }
      PropertyBindingFactory()
        .CreateCustomPropertyBinding(propertyName)
        ->Set(info.Env(), *this, formId, value);
    }

    return info.Env().Undefined();
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::Place(const Napi::CallbackInfo& info)
{
  try {
    auto globalRecordId = NapiHelper::ExtractUInt32(info[0], "globalRecordId");
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

    return Napi::Number::New(info.Env(), refr.GetFormId());
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::LookupEspmRecordById(const Napi::CallbackInfo& info)
{
  try {
    auto globalRecordId = NapiHelper::ExtractUInt32(info[0], "globalRecordId");

    auto espmLookupResult = Napi::Object::New(info.Env());

    auto lookupRes =
      partOne->GetEspm().GetBrowser().LookupById(globalRecordId);
    if (lookupRes.rec) {
      auto& cache = partOne->worldState.GetEspmCache();

      std::vector<Napi::Value> fields;
      espm::IterateFields_(
        lookupRes.rec,
        [&](const char* type, uint32_t size, const char* data) {
          auto uint8arr = Napi::Uint8Array::New(info.Env(), size);
          memcpy(uint8arr.Data(), data, size);

          auto field = Napi::Object::New(info.Env());
          field.Set("type",
                    Napi::String::New(info.Env(), std::string(type, 4)));
          field.Set("data", uint8arr);
          fields.push_back(field);
        },
        cache);
      auto fieldsArray = Napi::Array::New(info.Env(), fields.size());
      for (size_t i = 0; i < fields.size(); i++) {
        fieldsArray.Set(i, fields[i]);
      }

      auto id = Napi::Number::New(info.Env(), lookupRes.rec->GetId());
      auto edid =
        Napi::String::New(info.Env(), lookupRes.rec->GetEditorId(cache));
      auto type =
        Napi::String::New(info.Env(), lookupRes.rec->GetType().ToString());
      auto flags = Napi::Number::New(info.Env(), lookupRes.rec->GetFlags());

      auto record = Napi::Object::New(info.Env());
      record.Set("id", id);
      record.Set("editorId", edid);
      record.Set("type", type);
      record.Set("flags", flags);
      record.Set("fields", fieldsArray);
      espmLookupResult.Set("record", record);

      espmLookupResult.Set("fileIndex",
                           Napi::Number::New(info.Env(), lookupRes.fileIdx));

      espmLookupResult.Set(
        "toGlobalRecordId",
        Napi::Function::New(
          info.Env(), [lookupRes](const Napi::CallbackInfo& info) {
            auto localRecordId =
              NapiHelper::ExtractUInt32(info[0], "localRecordId");
            uint32_t res = lookupRes.ToGlobalId(localRecordId);
            return Napi::Number::New(info.Env(), res);
          }));
    }

    return espmLookupResult;
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::GetEspmLoadOrder(const Napi::CallbackInfo& info)
{
  try {
    auto fileNames = partOne->GetEspm().GetFileNames();
    auto arr = Napi::Array::New(info.Env(), fileNames.size());
    for (int i = 0; i < static_cast<int>(fileNames.size()); ++i) {
      arr.Set(i, Napi::String::New(info.Env(), fileNames[i]));
    }
    return arr;
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::GetDescFromId(const Napi::CallbackInfo& info)
{
  try {
    auto formId = NapiHelper::ExtractUInt32(info[0], "formId");
    auto espmFileNames = partOne->GetEspm().GetFileNames();
    auto formDesc = FormDesc::FromFormId(formId, espmFileNames);

    return Napi::String::New(info.Env(), formDesc.ToString());
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::GetIdFromDesc(const Napi::CallbackInfo& info)
{
  try {
    auto formDescStr = NapiHelper::ExtractString(info[0], "formDesc");
    auto formDesc = FormDesc::FromString(formDescStr);
    auto espmFileNames = partOne->GetEspm().GetFileNames();

    return Napi::Number::New(info.Env(), formDesc.ToFormId(espmFileNames));
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::CallPapyrusFunction(const Napi::CallbackInfo& info)
{
  try {
    auto callType = NapiHelper::ExtractString(info[0], "callType");
    auto className = NapiHelper::ExtractString(info[1], "className");
    auto functionName = NapiHelper::ExtractString(info[2], "functionName");
    auto self = PapyrusUtils::GetPapyrusValueFromJsValue(info[3], false,
                                                         partOne->worldState);

    auto arr = NapiHelper::ExtractArray(info[4], "args");
    auto arrSize = arr.Length();

    bool treatNumberAsInt = false; // TODO?

    std::vector<VarValue> args;
    args.resize(arrSize);
    for (int i = 0; i < arrSize; ++i) {
      args[i] = PapyrusUtils::GetPapyrusValueFromJsValue(
        arr.Get(i), treatNumberAsInt, partOne->worldState);
    }

    VarValue res;

    auto& vm = partOne->worldState.GetPapyrusVm();
    if (callType == "method") {
      res = vm.CallMethod(static_cast<IGameObject*>(self), functionName.data(),
                          args);
    } else if (callType == "global") {
      res = vm.CallStatic(className, functionName, args);
    } else {
      throw std::runtime_error("Unknown call type '" + callType +
                               "', expected one of ['method', 'global']");
    }

    return PapyrusUtils::GetJsValueFromPapyrusValue(
      info.Env(), res, partOne->worldState.espmFiles);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::RegisterPapyrusFunction(
  const Napi::CallbackInfo& info)
{
  try {
    auto callType = NapiHelper::ExtractString(info[0], "callType");
    auto className = NapiHelper::ExtractString(info[1], "className");
    auto functionName = NapiHelper::ExtractString(info[2], "functionName");
    auto f = NapiHelper::ExtractFunction(info[3], "f");

    std::shared_ptr<Napi::Reference<Napi::Function>> functionRef(
      new Napi::Reference<Napi::Function>(
        Napi::Persistent<Napi::Function>(f)));

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
      [functionRef, wst, env = info.Env()](const VarValue& self,
                                           const std::vector<VarValue>& args) {
        Napi::Value jsSelf =
          PapyrusUtils::GetJsValueFromPapyrusValue(env, self, wst->espmFiles);

        auto jsArgs = Napi::Array::New(env, args.size());
        for (int i = 0; i < static_cast<int>(args.size()); ++i) {
          jsArgs.Set(i,
                     PapyrusUtils::GetJsValueFromPapyrusValue(env, args[i],
                                                              wst->espmFiles));
        }

        auto jsResult = functionRef->Value().Call({ jsSelf, jsArgs });

        bool treatResultAsInt = false; // TODO?

        return PapyrusUtils::GetPapyrusValueFromJsValue(
          jsResult, treatResultAsInt, *wst);
      });

    return info.Env().Undefined();
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::SetPacketHistoryRecording(
  const Napi::CallbackInfo& info)
{
  try {
    auto userId = NapiHelper::ExtractUInt32(info[0], "userId");
    bool isRecording = NapiHelper::ExtractBoolean(info[1], "isRecording");
    partOne->SetPacketHistoryRecording(userId, isRecording);
    return info.Env().Undefined();
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::GetPacketHistory(const Napi::CallbackInfo& info)
{
  try {
    auto userId = NapiHelper::ExtractUInt32(info[0], "userId");
    auto history = partOne->GetPacketHistory(userId);
    return PacketHistoryWrapper::ToNapiValue(history, info.Env());
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::ClearPacketHistory(const Napi::CallbackInfo& info)
{
  try {
    auto userId = NapiHelper::ExtractUInt32(info[0], "userId");
    partOne->ClearPacketHistory(userId);
    return info.Env().Undefined();
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::RequestPacketHistoryPlayback(
  const Napi::CallbackInfo& info)
{
  try {
    auto userId = NapiHelper::ExtractUInt32(info[0], "userId");
    auto packetHistory = NapiHelper::ExtractObject(info[1], "packetHistory");

    PacketHistory history = PacketHistoryWrapper::FromNapiValue(packetHistory);

    partOne->RequestPacketHistoryPlayback(userId, history);
    return info.Env().Undefined();
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}
