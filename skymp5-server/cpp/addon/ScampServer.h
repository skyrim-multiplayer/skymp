#pragma once
#include "GamemodeApi.h"
#include "LocalizationProvider.h"
#include "Networking.h"
#include "NetworkingMock.h"
#include "PartOne.h"
#include "ScampServerListener.h"
#include <memory>
#include <napi.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

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
  Napi::Value CreateBot(const Napi::CallbackInfo& info);
  Napi::Value GetUserByActor(const Napi::CallbackInfo& info);
  Napi::Value WriteLogs(const Napi::CallbackInfo& info);

  Napi::Value GetLocalizedString(const Napi::CallbackInfo& info);
  Napi::Value GetServerSettings(const Napi::CallbackInfo& info);
  Napi::Value Clear(const Napi::CallbackInfo& info);
  Napi::Value MakeProperty(const Napi::CallbackInfo& info);
  Napi::Value MakeEventSource(const Napi::CallbackInfo& info);
  Napi::Value Get(const Napi::CallbackInfo& info);
  Napi::Value Set(const Napi::CallbackInfo& info);
  Napi::Value Place(const Napi::CallbackInfo& info);
  Napi::Value LookupEspmRecordById(const Napi::CallbackInfo& info);
  Napi::Value GetEspmLoadOrder(const Napi::CallbackInfo& info);
  Napi::Value GetDescFromId(const Napi::CallbackInfo& info);
  Napi::Value GetIdFromDesc(const Napi::CallbackInfo& info);
  Napi::Value CallPapyrusFunction(const Napi::CallbackInfo& info);
  Napi::Value RegisterPapyrusFunction(const Napi::CallbackInfo& info);
  Napi::Value SendCustomPacket(const Napi::CallbackInfo& info);

  Napi::Value SetPacketHistoryRecording(const Napi::CallbackInfo& info);
  Napi::Value GetPacketHistory(const Napi::CallbackInfo& info);
  Napi::Value ClearPacketHistory(const Napi::CallbackInfo& info);
  Napi::Value RequestPacketHistoryPlayback(const Napi::CallbackInfo& info);

  const std::shared_ptr<PartOne>& GetPartOne() const { return partOne; }
  const GamemodeApi::State& GetGamemodeApiState() const
  {
    return gamemodeApiState;
  }

private:
  std::shared_ptr<PartOne> partOne;
  std::shared_ptr<Networking::IServer> server;
  std::shared_ptr<Networking::MockServer> serverMock;
  std::shared_ptr<ScampServerListener> listener;
  Napi::Env tickEnv;
  Napi::ObjectReference emitter;
  Napi::FunctionReference emit;
  std::shared_ptr<spdlog::logger> logger;
  nlohmann::json serverSettings;
  GamemodeApi::State gamemodeApiState;

  std::shared_ptr<LocalizationProvider> localizationProvider;

  static Napi::FunctionReference constructor;
};
