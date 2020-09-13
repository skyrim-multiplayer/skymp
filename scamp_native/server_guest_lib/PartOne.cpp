#include "PartOne.h"
#include "ActionListener.h"
#include "Exceptions.h"
#include "IdManager.h"
#include "JsonUtils.h"
#include "MsgType.h"
#include "PacketParser.h"
#include "SqliteStorage.h"
#include <array>
#include <cassert>
#include <type_traits>
#include <vector>

struct PartOne::Impl
{
  simdjson::dom::parser parser;
  std::vector<std::shared_ptr<Listener>> listeners;
  espm::Loader* espm = nullptr;

  std::function<void(Networking::ISendTarget*sendTarget,
                     MpObjectReference*emitter, MpObjectReference*listener)>
    onSubscribe, onUnsubscribe;

  espm::CompressedFieldsCache compressedFieldsCache;
  bool enableProductionHacks = false;

  std::shared_ptr<PacketParser> packetParser;
  std::shared_ptr<IActionListener> actionListener;
};

PartOne::PartOne()
{
  pImpl.reset(new Impl);

  pImpl->onSubscribe = [this](Networking::ISendTarget* sendTarget,
                              MpObjectReference* emitter,
                              MpObjectReference* listener) {
    if (!emitter)
      throw std::runtime_error("nullptr emitter in onSubscribe");

    auto& emitterPos = emitter->GetPos();
    auto& emitterRot = emitter->GetAngle();

    bool isMe = emitter == listener;

    auto emitterAsActor = dynamic_cast<MpActor*>(emitter);

    std::string jEquipment, jLook;

    const char *lookPrefix = "", *look = "";
    if (emitterAsActor) {
      jLook = emitterAsActor->GetLookAsJson();
      if (!jLook.empty()) {
        lookPrefix = R"(, "look": )";
        look = jLook.data();
      }
    }

    const char *equipmentPrefix = "", *equipment = "";
    if (emitterAsActor) {
      jEquipment = emitterAsActor->GetEquipmentAsJson();
      if (!jEquipment.empty()) {
        equipmentPrefix = R"(, "equipment": )";
        equipment = jEquipment.data();
      }
    }

    const char* refrIdPrefix = "";
    char refrId[32] = { 0 };
    if (!emitterAsActor) {
      refrIdPrefix = R"(, "refrId": )";
      sprintf(refrId, "%d", emitter->GetFormId());
    }

    const char* baseIdPrefix = "";
    char baseId[32] = { 0 };
    if (emitter->GetBaseId() != 0) {
      baseIdPrefix = R"(, "baseId": )";
      sprintf(baseId, "%d", emitter->GetBaseId());
    }

    std::string props;

    const char *propsPrefix = "", *propsPostfix = "";
    emitter->VisitProperties([&](const char* propName, const char* jsonValue) {
      propsPrefix = R"(, "props": { )";
      propsPostfix = R"( })";

      if (props.size() > 0)
        props += R"(, ")";
      else
        props += '"';
      props += propName;
      props += R"(": )";
      props += jsonValue;
    });

    const char* method = "createActor";

    auto listenerAsActor = dynamic_cast<MpActor*>(listener);
    if (listenerAsActor) {
      auto listenerUserId = serverState.UserByActor(listenerAsActor);
      if (listenerUserId != Networking::InvalidUserId)
        Networking::SendFormatted(
          sendTarget, listenerUserId,
          R"({"type": "%s", "idx": %u, "isMe": %s, "transform": {"pos":
    [%f,%f,%f], "rot": [%f,%f,%f], "worldOrCell": %u}%s%s%s%s%s%s%s%s%s%s%s})",
          method, emitter->GetIdx(), isMe ? "true" : "false", emitterPos.x,
          emitterPos.y, emitterPos.z, emitterRot.x, emitterRot.y, emitterRot.z,
          emitter->GetCellOrWorld(), lookPrefix, look, equipmentPrefix,
          equipment, refrIdPrefix, refrId, baseIdPrefix, baseId, propsPrefix,
          props.data(), propsPostfix);
    }
  };

  pImpl->onUnsubscribe = [this](Networking::ISendTarget* sendTarget,
                                MpObjectReference* emitter,
                                MpObjectReference* listener) {
    auto listenerAsActor = dynamic_cast<MpActor*>(listener);
    if (!listenerAsActor)
      return;

    auto listenerUserId = serverState.UserByActor(listenerAsActor);
    if (listenerUserId != Networking::InvalidUserId &&
        listenerUserId != serverState.disconnectingUserId)
      Networking::SendFormatted(sendTarget, listenerUserId,
                                R"({"type": "destroyActor", "idx": %u})",
                                emitter->GetIdx());
  };
}

PartOne::PartOne(std::shared_ptr<Listener> listener)
  : PartOne()
{
  AddListener(listener);
}

PartOne::~PartOne()
{
  // worldState may depend on serverState (actorsMap), we should reset it first
  worldState.Clear();
  serverState = {};
}

void PartOne::AddListener(std::shared_ptr<Listener> listener)
{
  pImpl->listeners.push_back(listener);
}

bool PartOne::IsConnected(Networking::UserId userId) const
{
  return serverState.IsConnected(userId);
}

void PartOne::Tick()
{
  worldState.TickTimers();
}

void PartOne::CreateActor(uint32_t formId, const NiPoint3& pos, float angleZ,
                          uint32_t cellOrWorld,
                          Networking::ISendTarget* sendTarget)
{
  auto cbs = CreateActorCallbacks(sendTarget);
  worldState.AddForm(std::unique_ptr<MpActor>(new MpActor(
                       { pos, { 0, 0, angleZ }, cellOrWorld }, cbs.subscribe,
                       cbs.unsubscribe, cbs.sendToUser)),
                     formId);

  if (pImpl->enableProductionHacks) {
    auto& ac = worldState.GetFormAt<MpActor>(formId);
    auto defaultItems = { 0x00013922, 0x0003619E, 0x00013921, 0x00013920,
                          0x00013EDC, 0x000136D5, 0x000136D4, 0x000136D6,
                          0x000135BA, 0x000F6F23, 0x0001397E, 0x0012EB7,
                          0x00013790, 0x00013982, 0x0001359D, 0x00013980,
                          0x000D3DEA, 0x000A6D7B, 0x000A6D7F, 0x0006F39B,
                          0x000D3DEA };
    std::vector<Inventory::Entry> entries;
    for (uint32_t item : defaultItems) {
      entries.push_back({ item, 1 });
    }
    ac.AddItems(entries);
  }
}

void PartOne::EnableProductionHacks()
{
  pImpl->enableProductionHacks = true;
}

void PartOne::SetUserActor(Networking::UserId userId, uint32_t actorFormId,
                           Networking::ISendTarget* sendTarget)
{
  serverState.EnsureUserExists(userId);

  if (actorFormId > 0) {
    auto& actor = worldState.GetFormAt<MpActor>(actorFormId);

    serverState.actorsMap.insert({ userId, &actor });

    // Hacky way to force self-subscribing
    actor.SetPos(actor.GetPos());
  } else {
    serverState.actorsMap.left.erase(userId);
  }
}

uint32_t PartOne::GetUserActor(Networking::UserId userId)
{
  serverState.EnsureUserExists(userId);

  auto actor = serverState.ActorByUser(userId);
  if (!actor)
    return 0;
  return actor->GetFormId();
}

void PartOne::DestroyActor(uint32_t actorFormId)
{
  std::shared_ptr<MpActor> destroyedForm;
  worldState.DestroyForm<MpActor>(actorFormId, &destroyedForm);

  serverState.actorsMap.right.erase(destroyedForm.get());
}

void PartOne::SetRaceMenuOpen(uint32_t actorFormId, bool open,
                              Networking::ISendTarget* sendTarget)
{
  auto& actor = worldState.GetFormAt<MpActor>(actorFormId);

  if (actor.IsRaceMenuOpen() == open)
    return;

  actor.SetRaceMenuOpen(open);

  auto userId = serverState.UserByActor(&actor);
  if (userId == Networking::InvalidUserId) {
    std::stringstream ss;
    ss << "Actor with id " << std::hex << actorFormId
       << " is not attached to any of users";
    throw std::runtime_error(ss.str());
  }

  Networking::SendFormatted(sendTarget, userId,
                            R"({"type": "setRaceMenuOpen", "open": %s})",
                            open ? "true" : "false");
}

void PartOne::SendCustomPacket(Networking::UserId userId,
                               const std::string& jContent,
                               Networking::ISendTarget* sendTarget)
{
  Networking::SendFormatted(sendTarget, userId,
                            R"({"type": "customPacket", "content":%s})",
                            jContent.data());
}

std::string PartOne::GetActorName(uint32_t actorFormId)
{
  auto& ac = worldState.GetFormAt<MpActor>(actorFormId);
  return ac.GetLook() ? ac.GetLook()->name : "Prisoner";
}

NiPoint3 PartOne::GetActorPos(uint32_t actorFormId)
{
  auto& ac = worldState.GetFormAt<MpActor>(actorFormId);
  return ac.GetPos();
}

namespace {
inline const NiPoint3& GetPos(const espm::REFR::LocationalData* locationalData)
{
  return *reinterpret_cast<const NiPoint3*>(locationalData->pos);
}

inline NiPoint3 GetRot(const espm::REFR::LocationalData* locationalData)
{
  static const auto g_pi = std::acos(-1.f);
  return { locationalData->rotRadians[0] / g_pi * 180.f,
           locationalData->rotRadians[1] / g_pi * 180.f,
           locationalData->rotRadians[2] / g_pi * 180.f };
}

}

void PartOne::AttachEspm(espm::Loader* espm,
                         Networking::ISendTarget* sendTarget)
{
  pImpl->espm = espm;
  pImpl->espm->GetBrowser();
  worldState.AttachEspm(espm);

  clock_t was = clock();

  auto refrRecords = espm->GetBrowser().GetRecordsByType("REFR");
  for (size_t i = 0; i < refrRecords.size(); ++i) {
    auto& subVector = refrRecords[i];
    auto mapping = espm->GetBrowser().GetMapping(i);

    printf("starting %d\n", static_cast<int>(i));

    for (auto& refrRecord : *subVector) {
      auto refr = reinterpret_cast<espm::REFR*>(refrRecord);
      auto data = refr->GetData();

      auto baseId = espm::GetMappedId(data.baseId, *mapping);
      auto base = espm->GetBrowser().LookupById(baseId);
      if (!base.rec)
        printf("baseId %x %p\n", baseId, base.rec);
      if (!base.rec)
        continue;

      espm::Type t = base.rec->GetType();
      if (!espm::IsItem(t) && t != "DOOR" && t != "CONT" &&
          (t != "FLOR" ||
           !reinterpret_cast<espm::FLOR*>(base.rec)->GetData().resultItem) &&
          (t != "TREE" ||
           !reinterpret_cast<espm::TREE*>(base.rec)->GetData().resultItem))
        continue;

      auto formId = espm::GetMappedId(refrRecord->GetId(), *mapping);
      auto locationalData = data.loc;

      auto world = espm::GetExteriorWorldGroup(refrRecord);
      auto cell = espm::GetCellGroup(refrRecord);

      uint32_t worldOrCell;

      if (!world || !world->GetParentWRLD(worldOrCell))
        worldOrCell = 0;

      if (!worldOrCell) {
        if (!cell->GetParentCELL(worldOrCell)) {
          printf("Anomally: refr without world/cell\n");
          continue;
        }
      }

      auto existing = i ? worldState.LookupFormById(formId).get()
                        : reinterpret_cast<MpForm*>(0);
      if (existing) {
        auto existingAsRefr = reinterpret_cast<MpObjectReference*>(existing);

        if (locationalData) {
          existingAsRefr->SetPos(GetPos(locationalData));
          existingAsRefr->SetAngle(GetRot(locationalData));
        }

      } else {
        if (!locationalData) {
          printf("Anomally: refr without locationalData\n");
          continue;
        }

        auto typeStr = t.ToString();
        worldState.AddForm(
          std::unique_ptr<MpObjectReference>(new MpObjectReference(
            { GetPos(locationalData), GetRot(locationalData), worldOrCell },
            [sendTarget, this](MpObjectReference* emitter,
                               MpObjectReference* listener) {
              return pImpl->onSubscribe(sendTarget, emitter, listener);
            },
            [sendTarget, this](MpObjectReference* emitter,
                               MpObjectReference* listener) {
              return pImpl->onUnsubscribe(sendTarget, emitter, listener);
            },
            baseId, typeStr.data())),
          formId, true);
      }
    }
  }

  printf("AttachEspm took %d ticks\n", int(clock() - was));
}

void PartOne::LoadChangeForms(const char* fileName,
                              Networking::ISendTarget* sendTarget)
{
  auto storage = MakeSqliteStorage(fileName);
  storage.get_all<MpChangeForm>();

  // for (auto& changeForm : storage.iterate<MpChangeForm>()) {
  // auto formId = changeForm.formDesc.ToFormId(GetEspm().GetFileNames());
  /*
  bool isCreatedForm = changeForm.formDesc.file.empty();

  if (espm::Type(changeForm.recordType) == "ACHR") {
    if (isCreatedForm) {
      CreateActor(formId, { 0, 0, 0 }, 0, 0x3c, sendTarget);
    }
    auto& ac = worldState.GetFormAt<MpActor>(formId);
    ac.ApplyChangeForm(changeForm);
  } else if (espm::Type(changeForm.recordType) == "REFR") {
    if (isCreatedForm) {
      auto cbs = CreateActorCallbacks(sendTarget);

      auto base = GetEspm().GetBrowser().LookupById(
        changeForm.baseDesc.ToFormId(GetEspm().GetFileNames()));

      std::string baseType = base.rec->GetType().ToString();

      if (base.rec)
        worldState.AddForm(
          std::unique_ptr<MpObjectReference>(new MpObjectReference(
            { { 0, 0, 0 }, { 0, 0, 0 }, 0x3c }, cbs.subscribe,
            cbs.unsubscribe,
            changeForm.baseDesc.ToFormId(GetEspm().GetFileNames()),
            baseType.data())),
          formId);
    }
  } else
    throw std::runtime_error("Unknown record type " +
                             (espm::Type(changeForm.recordType).ToString()));*/
  //}
}

espm::Loader& PartOne::GetEspm() const
{
  return worldState.GetEspm();
}

namespace {
class ScopedTask
{
public:
  ScopedTask(std::function<void()> f_)
    : f(f_)
  {
  }
  ~ScopedTask() { f(); }

private:
  const std::function<void()> f;
};
}

void PartOne::HandlePacket(void* partOneInstance, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length)
{
  auto this_ = reinterpret_cast<PartOne*>(partOneInstance);

  switch (packetType) {
    case Networking::PacketType::ServerSideUserConnect:
      return this_->AddUser(userId, UserType::User);
    case Networking::PacketType::ServerSideUserDisconnect: {
      ScopedTask t([userId, this_] {
        this_->serverState.Disconnect(userId);
        this_->serverState.disconnectingUserId = Networking::InvalidUserId;
      });

      this_->serverState.disconnectingUserId = userId;
      for (auto& listener : this_->pImpl->listeners)
        listener->OnDisconnect(userId);
      return;
    }
    case Networking::PacketType::Message:
      return this_->HandleMessagePacket(userId, data, length);
    default:
      throw std::runtime_error("Unexpected PacketType: " +
                               std::to_string((int)packetType));
  }
}

PartOne::ActorCallbacks PartOne::CreateActorCallbacks(
  Networking::ISendTarget* sendTarget)
{
  auto st = &serverState;

  MpActor::SubscribeCallback subscribe =
                               [sendTarget, this](MpObjectReference*emitter,
                                                  MpObjectReference*listener) {
                                 return pImpl->onSubscribe(sendTarget, emitter,
                                                           listener);
                               },
                             unsubscribe = [sendTarget,
                                            this](MpObjectReference*emitter,
                                                  MpObjectReference*listener) {
                               return pImpl->onUnsubscribe(sendTarget, emitter,
                                                           listener);
                             };

  MpActor::SendToUserFn sendToUser = [sendTarget,
                                      st](MpActor* actor, const void* data,
                                          size_t size, bool reliable) {
    auto targetuserId = st->UserByActor(actor);
    if (targetuserId != Networking::InvalidUserId &&
        st->disconnectingUserId != targetuserId)
      sendTarget->Send(targetuserId,
                       reinterpret_cast<Networking::PacketData>(data), size,
                       reliable);
  };

  return { subscribe, unsubscribe, sendToUser };
}

void PartOne::AddUser(Networking::UserId userId, UserType type)
{
  serverState.Connect(userId);
  for (auto& listener : pImpl->listeners)
    listener->OnConnect(userId);
}

void PartOne::HandleMessagePacket(Networking::UserId userId,
                                  Networking::PacketData data, size_t length)
{
  if (!serverState.IsConnected(userId))
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");
  if (!pImpl->packetParser)
    pImpl->packetParser.reset(new PacketParser);

  if (!pImpl->actionListener)
    pImpl->actionListener.reset(
      new ActionListener(worldState, serverState, pImpl->listeners,
                         pImpl->espm, pushedSendTarget));

  pImpl->packetParser->TransformPacketIntoAction(userId, data, length,
                                                 *pImpl->actionListener);
}