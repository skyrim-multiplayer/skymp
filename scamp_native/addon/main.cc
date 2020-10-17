#include "Networking.h"
#include "NetworkingCombined.h"
#include "NetworkingMock.h"
#include "PartOne.h"
#include "SqliteSaveStorage.h"
#include <cassert>
#include <memory>
#include <napi.h>

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

  Napi::Value Tick(const Napi::CallbackInfo& info);
  Napi::Value On(const Napi::CallbackInfo& info);
  Napi::Value CreateActor(const Napi::CallbackInfo& info);
  Napi::Value SetUserActor(const Napi::CallbackInfo& info);
  Napi::Value GetUserActor(const Napi::CallbackInfo& info);
  Napi::Value GetActorPos(const Napi::CallbackInfo& info);
  Napi::Value GetActorName(const Napi::CallbackInfo& info);
  Napi::Value DestroyActor(const Napi::CallbackInfo& info);
  Napi::Value SetRaceMenuOpen(const Napi::CallbackInfo& info);
  Napi::Value SendCustomPacket(const Napi::CallbackInfo& info);
  Napi::Value CreateBot(const Napi::CallbackInfo& info);

private:
  std::unique_ptr<PartOne> partOne;
  std::shared_ptr<Networking::IServer> server;
  std::shared_ptr<Networking::MockServer> serverMock;
  std::shared_ptr<ScampServerListener> listener;
  Napi::Env tickEnv;
  Napi::ObjectReference emitter;
  Napi::FunctionReference emit;

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

private:
  ScampServer& server;
};

Napi::FunctionReference ScampServer::constructor;

Napi::Object ScampServer::Init(Napi::Env env, Napi::Object exports)
{
  Napi::Function func = DefineClass(
    env, "ScampServer",
    { InstanceMethod<&ScampServer::Tick>("tick"),
      InstanceMethod<&ScampServer::On>("on"),
      InstanceMethod<&ScampServer::CreateActor>("createActor"),
      InstanceMethod<&ScampServer::SetUserActor>("setUserActor"),
      InstanceMethod<&ScampServer::GetUserActor>("getUserActor"),
      InstanceMethod<&ScampServer::GetActorPos>("getActorPos"),
      InstanceMethod<&ScampServer::GetActorName>("getActorName"),
      InstanceMethod<&ScampServer::DestroyActor>("destroyActor"),
      InstanceMethod<&ScampServer::SetRaceMenuOpen>("setRaceMenuOpen"),
      InstanceMethod<&ScampServer::SendCustomPacket>("sendCustomPacket"),
      InstanceMethod<&ScampServer::CreateBot>("createBot") });
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

    auto espm = new espm::Loader(dataDir,
                                 { "Skyrim.esm", "Update.esm", "Dawnguard.esm",
                                   "HearthFires.esm", "Dragonborn.esm" });
    auto realServer = Networking::CreateServer(
      static_cast<uint32_t>(port), static_cast<uint32_t>(maxConnections));
    server = Networking::CreateCombinedServer({ realServer, serverMock });
    partOne->AttachEspm(espm, server.get());
    partOne->AttachSaveStorage(
      std::make_shared<SqliteSaveStorage>("world.sqlite"),
      server.get()); // TODO

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

Napi::Value ScampServer::Tick(const Napi::CallbackInfo& info)
{
  try {
    tickEnv = info.Env();
    partOne->pushedSendTarget = server.get();
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
  try {
    partOne->CreateActor(formId, pos, angleZ, cellOrWorld, server.get());
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::SetUserActor(const Napi::CallbackInfo& info)
{
  auto userId = info[0].As<Napi::Number>().Uint32Value();
  auto actorFormId = info[1].As<Napi::Number>().Uint32Value();
  try {
    partOne->SetUserActor(userId, actorFormId, server.get());
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
    partOne->SetRaceMenuOpen(formId, open, server.get());
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
    partOne->SendCustomPacket(userId, string, server.get());
  } catch (std::exception& e) {
    throw Napi::Error::New(info.Env(), (std::string)e.what());
  }
  return info.Env().Undefined();
}

Napi::Value ScampServer::CreateBot(const Napi::CallbackInfo& info)
{
  if (!this->serverMock)
    throw Napi::Error::New(info.Env(), "Bad serverMock");

  std::shared_ptr<Bot> bot(new Bot(this->serverMock->CreateClient()));

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
