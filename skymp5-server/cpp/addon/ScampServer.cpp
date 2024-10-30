#include "ScampServer.h"

#include "Bot.h"
#include "FormCallbacks.h"
#include "GamemodeApi.h"
#include "NapiHelper.h"
#include "NetworkingCombined.h"
#include "PacketHistoryWrapper.h"
#include "PapyrusUtils.h"
#include "ScampServerListener.h"
#include "database_drivers/DatabaseFactory.h"
#include "formulas/DamageMultFormula.h"
#include "formulas/SweetPieDamageFormula.h"
#include "formulas/TES5DamageFormula.h"
#include "libespm/IterateFields.h"
#include "papyrus-vm/Utils.h"
#include "property_bindings/PropertyBindingFactory.h"
#include "save_storages/SaveStorageFactory.h"
#include "script_objects/EspmGameObject.h"
#include "script_storages/ScriptStorageFactory.h"
#include <cassert>
#include <cctype>
#include <memory>
#include <napi.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sstream>

namespace {

constexpr size_t kMockServerIdx = 1;

std::shared_ptr<spdlog::logger>& GetLogger()
{
  static auto g_logger = spdlog::stdout_color_mt("console");
  return g_logger;
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

bool StartsWith(const std::string& str, const char* prefix)
{
  return str.compare(0, strlen(prefix), prefix) == 0;
}

}

Napi::FunctionReference ScampServer::constructor;

Napi::Object ScampServer::Init(Napi::Env env, Napi::Object exports)
{
  Napi::Function func = DefineClass(
    env, "ScampServer",
    { InstanceMethod("_setSelf", &ScampServer::_SetSelf),
      InstanceMethod("attachSaveStorage", &ScampServer::AttachSaveStorage),
      InstanceMethod("tick", &ScampServer::Tick),
      InstanceMethod("on", &ScampServer::On),
      InstanceMethod("createActor", &ScampServer::CreateActor),
      InstanceMethod("setUserActor", &ScampServer::SetUserActor),
      InstanceMethod("getUserActor", &ScampServer::GetUserActor),
      InstanceMethod("getUserGuid", &ScampServer::GetUserGuid),
      InstanceMethod("isConnected", &ScampServer::IsConnected),
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
      InstanceMethod("getUserIp", &ScampServer::GetUserIp),

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
      InstanceMethod("getNeighborsByPosition",
                     &ScampServer::GetNeighborsByPosition),
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
                     &ScampServer::RequestPacketHistoryPlayback),
      InstanceMethod("findFormsByPropertyValue",
                     &ScampServer::FindFormsByPropertyValue),

      InstanceMethod("_sp3ListClasses", &ScampServer::SP3ListClasses),
      InstanceMethod("_sp3GetBaseClass", &ScampServer::SP3GetBaseClass),
      InstanceMethod("_sp3ListStaticFunctions",
                     &ScampServer::SP3ListStaticFunctions),
      InstanceMethod("_sp3ListMethods", &ScampServer::SP3ListMethods),
      InstanceMethod("_sp3GetFunctionImplementation",
                     &ScampServer::SP3GetFunctionImplementation),
      InstanceMethod("_sp3DynamicCast", &ScampServer::SP3DynamicCast) });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("ScampServer", func);

  exports.Set("writeLogs", Napi::Function::New(env, WriteLogs));
  return exports;
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

    std::string serverSettingsJson =
      static_cast<std::string>(info[2].As<Napi::String>());

    serverMock = std::make_shared<Networking::MockServer>();

    std::string dataDir;

    const auto& logger = GetLogger();
    partOne->AttachLogger(logger);

    auto serverSettings = nlohmann::json::parse(serverSettingsJson);

    if (serverSettings.find("weaponStaminaModifiers") !=
        serverSettings.end()) {
      if (serverSettings.at("weaponStaminaModifiers").is_object()) {
        auto modifiers = serverSettings.at("weaponStaminaModifiers")
                           .get<std::unordered_map<std::string, float>>();
        if (modifiers.empty()) {
          logger->info("\"weaponStaminaModifiers field is empty. Using "
                       "default values for stamina managment instead.\"");
        } else {
          logger->info(
            "Using keywords based stamina forfeits for players attacks");
        }
        partOne->animationSystem.SetWeaponStaminaModifiers(
          std::move(modifiers));
      }
    } else {
      logger->info("\"weaponStaminaModifiers field is missing. Using "
                   "default values for stamina managment instead.\"");
    }

    if (serverSettings["logLevel"].is_string()) {
      const auto level = spdlog::level::from_str(serverSettings["logLevel"]);
      logger->set_level(level);
      spdlog::set_level(level);
      logger->info("set log level to {}",
                   spdlog::level::to_string_view(logger->level()));
    }

    if (serverSettings.find("npcEnabled") != serverSettings.end()) {
      bool enabled = serverSettings.at("npcEnabled").get<bool>();
      partOne->worldState.npcEnabled = enabled;
      if (enabled) {
        spdlog::info("NPCs are enabled");
      } else {
        spdlog::info("NPCs are disabled");
      }
    } else {
      spdlog::info(
        "npcEnabled option is not found in the server configuration file. "
        "Disabling NPCs by default");
    }

    if (serverSettings.find("npcSettings") != serverSettings.end()) {
      if (serverSettings.at("npcSettings").is_object()) {
        std::unordered_map<std::string, WorldState::NpcSettingsEntry>
          npcSettings;
        if (serverSettings.find("default") != serverSettings.end()) {
          partOne->worldState.defaultSetting.spawnInInterior =
            serverSettings.at("spawnInInterior").get<bool>();
          partOne->worldState.defaultSetting.spawnInExterior =
            serverSettings.at("spawnInExterior").get<bool>();
          partOne->worldState.defaultSetting.overriden = true;
        }
        for (const auto& field : serverSettings["npcSettings"].items()) {
          WorldState::NpcSettingsEntry entry;
          if (field.value().find("spawnInInterior") != field.value().end()) {
            entry.spawnInInterior =
              field.value().at("spawnInInterior").get<bool>();
          }
          if (field.value().find("spawnInExterior") != field.value().end()) {
            entry.spawnInExterior =
              field.value().at("spawnInExterior").get<bool>();
          }
          npcSettings[field.key()] = entry;
        }
        partOne->worldState.SetNpcSettings(std::move(npcSettings));
        spdlog::info("NPCs' settings have been loaded susccessfully");
      }
    } else {
      std::stringstream msg;
      msg << "\"npcSettings\" weren't found in the server configuration "
             "file.";
      msg << (partOne->worldState.npcEnabled
                ? "Allowing all npc by default"
                : "NPCs are disabled due to \"npcEnabled\": ")
          << std::boolalpha << partOne->worldState.npcEnabled;
      spdlog::info(msg.str());
    }

    if (serverSettings.find("enableConsoleCommandsForAll") !=
        serverSettings.end()) {
      if (serverSettings.at("enableConsoleCommandsForAll").is_boolean()) {
        bool enableConsoleCommandsForAll =
          serverSettings["enableConsoleCommandsForAll"].get<bool>();
        spdlog::info("enableConsoleCommandsForAll is explicitly set to {}",
                     enableConsoleCommandsForAll);
        partOne->worldState.SetEnableConsoleCommandsForAllSetting(
          enableConsoleCommandsForAll);
      } else {
        spdlog::error("Unexpected value of enableConsoleCommandsForAll "
                      "setting, should be true or false");
      }
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
      ? std::string(kNetworkingPasswordPrefix) +
        static_cast<std::string>(serverSettings["password"])
      : std::string(kNetworkingPasswordPrefix);
    auto realServer = Networking::CreateServer(
      static_cast<uint32_t>(port), static_cast<uint32_t>(maxConnections),
      password.data());

    static_assert(kMockServerIdx == 1);
    server = Networking::CreateCombinedServer({ realServer, serverMock });

    partOne->SetSendTarget(server.get());

    auto sweetPieDamageFormulaSettings =
      serverSettings["sweetPieDamageFormulaSettings"];

    auto damageMultFormulaSettings =
      serverSettings["damageMultFormulaSettings"];

    std::unique_ptr<IDamageFormula> formula;
    formula = std::make_unique<TES5DamageFormula>();
    formula = std::make_unique<DamageMultFormula>(std::move(formula),
                                                  damageMultFormulaSettings);
    formula = std::make_unique<SweetPieDamageFormula>(
      std::move(formula), sweetPieDamageFormulaSettings);
    partOne->SetDamageFormula(std::move(formula));

    partOne->worldState.AttachScriptStorage(
      ScriptStorageFactory::Create(serverSettings));

    partOne->AttachEspm(espm);
    partOne->animationSystem.Init(&partOne->worldState);

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

    auto it = serverSettings.find("forbiddenReloot");
    if (it != serverSettings.end() && (*it).is_array()) {
      partOne->worldState.SetForbiddenRelootTypes(
        (*it).get<std::set<std::string>>());
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

Napi::Value ScampServer::_SetSelf(const Napi::CallbackInfo& info)
{
  try {
    auto self = info[0].As<Napi::Object>();
    auto persistent = Napi::Persistent(self);
    this->self = std::move(persistent);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::AttachSaveStorage(const Napi::CallbackInfo& info)
{
  try {
    auto db = DatabaseFactory::Create(serverSettings, logger);
    auto saveStorage = SaveStorageFactory::Create(db, logger);
    partOne->AttachSaveStorage(saveStorage);
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
  try {
    auto formId = NapiHelper::ExtractUInt32(info[0], "formId");
    auto pos = NapiHelper::ExtractNiPoint3(info[1], "pos");
    auto angleZ = NapiHelper::ExtractFloat(info[2], "angleZ");
    auto cellOrWorld = NapiHelper::ExtractUInt32(info[3], "cellOrWorld");

    int32_t userProfileId = -1;
    if (info[4].IsNumber()) {
      userProfileId = info[4].As<Napi::Number>().Int32Value();
    }

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

Napi::Value ScampServer::GetUserGuid(const Napi::CallbackInfo& info)
{
  auto userId = info[0].As<Napi::Number>().Uint32Value();
  try {
    return Napi::String::New(info.Env(), partOne->GetUserGuid(userId));
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::IsConnected(const Napi::CallbackInfo& info)
{
  auto userId = info[0].As<Napi::Number>().Uint32Value();
  try {
    return Napi::Boolean::New(info.Env(), partOne->IsConnected(userId));
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

Napi::Value ScampServer::GetUserIp(const Napi::CallbackInfo& info)
{
  try {
    auto userId = info[0].As<Napi::Number>().Uint32Value();
    return Napi::String::New(info.Env(), server->GetIp(userId));
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
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
    if (parsedServerSettings.IsEmpty()) {
      parsedServerSettings =
        Napi::Persistent(NapiHelper::ParseJson(info.Env(), serverSettings));
    }
    return parsedServerSettings.Value();
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

Napi::Value ScampServer::GetNeighborsByPosition(const Napi::CallbackInfo& info)
{
  try {
    auto cellOrWorldDesc = FormDesc::FromString(
      NapiHelper::ExtractString(info[0], "cellOrWorldDesc"));
    auto pos = NapiHelper::ExtractNiPoint3(info[1], "pos");

    auto cellX = static_cast<int32_t>(pos[0] / 4096);
    auto cellY = static_cast<int32_t>(pos[1] / 4096);
    auto& refs = partOne->worldState.GetNeighborsByPosition(
      cellOrWorldDesc.ToFormId(partOne->worldState.espmFiles), cellX, cellY);

    Napi::Array arr = Napi::Array::New(info.Env(), refs.size());
    for (auto ref : refs) {
      arr.Set(arr.Length(), Napi::Number::New(info.Env(), ref->GetFormId()));
    }
    return arr;
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

namespace {
VarValue CallPapyrusFunctionImpl(const std::shared_ptr<PartOne>& partOne,
                                 int callType, const std::string& className,
                                 const std::string& functionName,
                                 const VarValue& self,
                                 std::vector<VarValue>& args)
{
  bool treatNumberAsInt = false; // TODO?

  VarValue res;

  auto& vm = partOne->worldState.GetPapyrusVm();
  if (callType == 'meth') {
    if (self.GetType() == VarValue::Type::kType_Object) {
      res = vm.CallMethod(static_cast<IGameObject*>(self), functionName.data(),
                          args);
    } else {
      throw std::runtime_error(
        "Can't call Papyrus method on non-object self '" + self.ToString() +
        "'");
    }
  } else if (callType == 'glob') {
    res = vm.CallStatic(className, functionName, args);
  } else {
    throw std::runtime_error(
      "Unknown call type, expected one of ['method', 'global']");
  }

  return res;
}
}

Napi::Value ScampServer::CallPapyrusFunction(const Napi::CallbackInfo& info)
{
  // This function throws exceptions in case of bad input
  // But it also catches exceptions from the Papyrus VM functions
  // This is because
  // 1) they're rare and unexpected, and we don't want to crash the sever
  // 2) in Papyrus (not JS) we catch them all. so it's consistent
  // 3) we plan replacing all exceptions with logs in Papyrus VM functions
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

    int callTypeInt = 0;

    if (callType == "method") {
      callTypeInt = 'meth';
    } else if (callType == "global") {
      callTypeInt = 'glob';
    } else {
      throw std::runtime_error("Unknown call type '" + callType +
                               "', expected one of ['method', 'global']");
    }

    auto res = CallPapyrusFunctionImpl(partOne, callTypeInt, className,
                                       functionName, self, args);

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

Napi::Value ScampServer::FindFormsByPropertyValue(
  const Napi::CallbackInfo& info)
{
  try {
    auto propertyName = NapiHelper::ExtractString(info[0], "propertyName");
    auto propertyValue = info[1];

    if (!StartsWith(propertyName,
                    MpObjectReference::GetPropertyPrefixPrivateIndexed())) {
      spdlog::error("FindFormsByPropertyValue - Attempt to search for "
                    "non-indexed property '{}'",
                    propertyName);
    }

    auto propertyValueStringified =
      NapiHelper::Stringify(info.Env(), propertyValue);

    auto mapKey = partOne->worldState.MakePrivateIndexedPropertyMapKey(
      propertyName, propertyValueStringified);

    auto& formIds =
      partOne->worldState.GetActorsByPrivateIndexedProperty(mapKey);
    auto result = Napi::Array::New(info.Env(), formIds.size());
    uint32_t i = 0;
    for (auto formId : formIds) {
      result.Set(i, Napi::Number::New(info.Env(), formId));
      ++i;
    }
    return result;
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::SP3ListClasses(const Napi::CallbackInfo& info)
{
  try {
    auto classNames = partOne->worldState.GetPapyrusVm().ListClasses();

    auto result = Napi::Array::New(info.Env(), 0);

    for (auto& className : classNames) {
      result.Set(result.Length(),
                 Napi::String::New(info.Env(), className.data()));
    }

    return result;
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::SP3GetBaseClass(const Napi::CallbackInfo& info)
{
  try {
    auto className = NapiHelper::ExtractString(info[0], "className");
    CIString baseClass =
      partOne->worldState.GetPapyrusVm().GetBaseClass(className.data());
    return Napi::String::New(info.Env(), baseClass.data());
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::SP3ListStaticFunctions(const Napi::CallbackInfo& info)
{
  try {
    auto className = NapiHelper::ExtractString(info[0], "className");
    auto staticFunctions =
      partOne->worldState.GetPapyrusVm().ListStaticFunctions(className.data());
    auto result = Napi::Array::New(info.Env(), staticFunctions.size());
    size_t i = 0;
    for (auto& staticFunction : staticFunctions) {
      result.Set(i, Napi::String::New(info.Env(), staticFunction.data()));
      ++i;
    }
    return result;
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::SP3ListMethods(const Napi::CallbackInfo& info)
{
  try {
    auto className = NapiHelper::ExtractString(info[0], "className");
    auto methods =
      partOne->worldState.GetPapyrusVm().ListMethods(className.data());
    auto result = Napi::Array::New(info.Env(), methods.size());
    size_t i = 0;
    for (auto& method : methods) {
      result.Set(i, Napi::String::New(info.Env(), method.data()));
      ++i;
    }
    return result;
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::SP3GetFunctionImplementation(
  const Napi::CallbackInfo& info)
{
  try {
    auto className = NapiHelper::ExtractString(info[1], "className");
    auto functionName = NapiHelper::ExtractString(info[2], "functionName");

    NativeFunction functionImplementationStatic, functionImplementationMethod;

    constexpr bool kIsStaticTrue = true;
    functionImplementationStatic =
      partOne->worldState.GetPapyrusVm().GetFunctionImplementation(
        className.data(), functionName.data(), kIsStaticTrue);

    constexpr bool kIsStaticFalse = false;
    functionImplementationMethod =
      partOne->worldState.GetPapyrusVm().GetFunctionImplementation(
        className.data(), functionName.data(), kIsStaticFalse);

    if (!functionImplementationStatic && !functionImplementationMethod) {
      return info.Env().Undefined();
    }

    auto function = [className, functionName, functionImplementationStatic,
                     functionImplementationMethod,
                     this](const Napi::CallbackInfo& info) -> Napi::Value {
      try {
        Napi::Value jsThis = info.This();

        // Hack to detect that this arg refers to class not to an object, so
        // it's a static call
        if (jsThis.IsObject()) {
          if (jsThis.As<Napi::Object>().Get("desc").IsUndefined()) {
            jsThis = info.Env().Undefined();
          }
        }

        std::vector<VarValue> args;

        for (size_t i = 0; i < info.Length(); ++i) {
          auto arg = PapyrusUtils::GetPapyrusValueFromJsValue(
            info[i], false, partOne->worldState);
          args.push_back(arg);
        }

        VarValue self = jsThis.IsUndefined()
          ? VarValue::None()
          : PapyrusUtils::GetPapyrusValueFromJsValue(jsThis, false,
                                                     partOne->worldState);

        int callType = jsThis.IsUndefined() ? 'glob' : 'meth';

        VarValue res = CallPapyrusFunctionImpl(partOne, callType, className,
                                               functionName, self, args);

        auto jsRes = PapyrusUtils::GetJsValueFromPapyrusValue(
          info.Env(), res, partOne->worldState.espmFiles);

        if (jsRes.IsObject()) {

          PexScript::Lazy pexScriptLazy =
            partOne->worldState.GetPapyrusVm().GetPexByName(className.data());

          if (pexScriptLazy.fn) {
            std::shared_ptr<PexScript> pexScript = pexScriptLazy.fn();

            if (pexScript) {
              auto& obj = pexScript->objectTable.at(0);
              auto& states = obj.states;
              auto& autoStateName = obj.autoStateName;
              auto stateIt = std::find_if(states.begin(), states.end(),
                                          [&](const auto& state) {
                                            return state.name == autoStateName;
                                          });
              if (stateIt != states.end()) {
                auto& state = *stateIt;
                auto& functions = state.functions;
                auto functionIt = std::find_if(
                  functions.begin(), functions.end(),
                  [&](const auto& function) {
                    return Utils::stricmp(function.name.data(),
                                          functionName.data()) == 0;
                  });
                if (functionIt != functions.end()) {
                  auto& function = functionIt->function;
                  jsRes.As<Napi::Object>().Set("_sp3ObjectType",
                                               function.returnType);
                }
              }
            }
          }
        }

        return jsRes;
      } catch (std::exception& e) {
        throw Napi::Error::New(info.Env(), std::string(e.what()));
      }
    };

    return Napi::Function::New(info.Env(), function);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Value ScampServer::SP3DynamicCast(const Napi::CallbackInfo& info)
{
  try {
    auto object = PapyrusUtils::GetPapyrusValueFromJsValue(
      info[0], false, partOne->worldState);
    auto className = NapiHelper::ExtractString(info[1], "className");

    bool result =
      partOne->worldState.GetPapyrusVm().DynamicCast(object, className.data());

    return Napi::Boolean::New(info.Env(), result);
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}
