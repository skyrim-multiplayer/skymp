#include "ScampServer.h"

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
      InstanceMethod("executeJavaScriptOnChakra",
                     &ScampServer::ExecuteJavaScriptOnChakra),
      InstanceMethod("writeLogs", &ScampServer::WriteLogs),

      InstanceMethod("getLocalizedString", &ScampServer::GetLocalizedString),
      InstanceMethod("getServerSettings", &ScampServer::GetServerSettings),
      InstanceMethod("clear", &ScampServer::Clear),
      InstanceMethod("makeProperty", &ScampServer::MakeProperty),
      InstanceMethod("makeEventSource", &ScampServer::MakeEventSource),
      InstanceMethod("get", &ScampServer::Get),
      InstanceMethod("set", &ScampServer::Set),
      InstanceMethod("place", &ScampServer::Place),
      InstanceMethod("lookupEspmRecordById", &ScampServer::LookupEspmRecordById),
      InstanceMethod("getEspmLoadOrder", &ScampServer::GetEspmLoadOrder),
      InstanceMethod("getDescFromId", &ScampServer::GetDescFromId),
      InstanceMethod("getIdFromDesc", &ScampServer::GetIdFromDesc),
      InstanceMethod("callPapyrusFunction", &ScampServer::CallPapyrusFunction),
      InstanceMethod("registerPapyrusFunction", &ScampServer::RegisterPapyrusFunction),
      InstanceMethod("sendCustomPacket", &ScampServer::SendCustomPacket)
       });
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
  if (!this->serverMock) {
    throw Napi::Error::New(info.Env(), "Bad serverMock");
  }

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

Napi::Value ScampServer::GetLocalizedString(const Napi::CallbackInfo &info) {
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

            translatedString = Napi::String::New(info.Env(), this->localizationProvider->Get(fileNameWithoutExt, stringId));
          }
        },
        cache);

      return translatedString;
  }
  catch(std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::GetServerSettings(const Napi::CallbackInfo &info) {
  try {
    return NapiHelper::ParseJson(info.Env(), serverSettings);
  }
  catch(std::exception &e) {
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

Napi::Value ScampServer::MakeProperty(const Napi::CallbackInfo &info) {
  try {
      static const auto kAlphabet = GetPropertyAlphabet();
      static const std::pair<size_t, size_t> kMinMaxSize = {1, 128};
      auto isUnique = [this] (const std::string &s) { 
        return gamemodeApiState.createdProperties.count(s) == 0; 
      };
      auto propertyName = NapiHelper::ExtractString(info[0], "propertyName", kAlphabet, kMinMaxSize, isUnique);

      GamemodeApi::PropertyInfo propertyInfo;

      auto options = NapiHelper::ExtractObject(info[1], "options");

      std::vector<std::pair<std::string, bool*>> booleans{
        { "isVisibleByOwner", &propertyInfo.isVisibleByOwner },
        { "isVisibleByNeighbors", &propertyInfo.isVisibleByNeighbors }
      };
      for (auto [optionName, ptr] : booleans) {
        std::string argName = "options." + optionName;
        auto v = NapiHelper::ExtractBoolean(options.Get(optionName.data()), argName.data());
        *ptr = static_cast<bool>(v);
      }

      std::vector<std::pair<std::string, std::string*>> strings{
        { "updateNeighbor", &propertyInfo.updateNeighbor },
        { "updateOwner", &propertyInfo.updateOwner }
      };
      for (auto [optionName, ptr] : strings) {
        std::string argName = "options." + optionName;
        static const std::pair<size_t, size_t> kMinMaxSize = {0, 100 * 1024};
        auto v = NapiHelper::ExtractString(options.Get(optionName.data()), argName.data(), std::nullopt, kMinMaxSize);
        *ptr = static_cast<std::string>(v);
      }

      gamemodeApiState.createdProperties[propertyName] = propertyInfo;
      partOne->NotifyGamemodeApiStateChanged(gamemodeApiState);

      return info.Env().Undefined();
  }
  catch(std::exception &e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::MakeEventSource(const Napi::CallbackInfo &info) {
  try {
    static const auto kAlphabet = GetPropertyAlphabet();
    static const std::pair<size_t, size_t> kMinMaxSizeEventName = {1, 128};
    auto isUnique = [this] (const std::string &s) { 
      return gamemodeApiState.createdEventSources.count(s) == 0; 
    };
    auto eventName = NapiHelper::ExtractString(info[0], "eventName", kAlphabet, kMinMaxSizeEventName);

    static const std::pair<size_t, size_t> kMinMaxSizeFunctionBody = {0, 100 * 1024};
    auto functionBody = NapiHelper::ExtractString(info[1], "functionBody", std::nullopt, kMinMaxSizeFunctionBody);
    gamemodeApiState.createdEventSources[eventName] = { functionBody };
    partOne->NotifyGamemodeApiStateChanged(gamemodeApiState);
  }
  catch(std::exception &e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}
