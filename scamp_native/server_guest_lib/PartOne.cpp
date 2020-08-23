#include "PartOne.h"
#include "Exceptions.h"
#include "IdManager.h"
#include "JsonUtils.h"
#include "MsgType.h"
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
                     MpObjectReference*emitter, MpActor*listener)>
    onSubscribe, onUnsubscribe;

  espm::CompressedFieldsCache compressedFieldsCache;
  bool enableProductionHacks = false;
};

PartOne::PartOne()
{
  pImpl.reset(new Impl);

  pImpl->onSubscribe = [this](Networking::ISendTarget* sendTarget,
                              MpObjectReference* emitter, MpActor* listener) {
    if (!emitter)
      throw std::runtime_error("nullptr emitter in onSubscribe");

    auto& emitterPos = emitter->GetPos();
    auto& emitterRot = emitter->GetAngle();

    bool isMe = emitter == listener;

    auto emitterAsActor = dynamic_cast<MpActor*>(emitter);

    const char *lookPrefix = "", *look = "";
    if (emitterAsActor) {
      auto& jLook = emitterAsActor->GetLookAsJson();
      if (!jLook.empty()) {
        lookPrefix = R"(, "look": )";
        look = jLook.data();
      }
    }

    const char *equipmentPrefix = "", *equipment = "";
    if (emitterAsActor) {
      auto& jEquipment = emitterAsActor->GetEquipmentAsJson();
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

    auto listenerUserId = serverState.UserByActor(listener);
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
  };

  pImpl->onUnsubscribe = [this](Networking::ISendTarget* sendTarget,
                                MpObjectReference* emitter,
                                MpActor* listener) {
    auto listenerUserId = serverState.UserByActor(listener);
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
  auto st = &serverState;

  MpActor::SubscribeCallback
    subscribe =
      [sendTarget, this](MpObjectReference*emitter, MpActor*listener) {
        return pImpl->onSubscribe(sendTarget, emitter, listener);
      },
    unsubscribe = [sendTarget, this](MpObjectReference*emitter,
                                     MpActor*listener) {
      return pImpl->onUnsubscribe(sendTarget, emitter, listener);
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

  worldState.AddForm(std::unique_ptr<MpActor>(
                       new MpActor({ pos, { 0, 0, angleZ }, cellOrWorld },
                                   subscribe, unsubscribe, sendToUser)),
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
            [sendTarget, this](MpObjectReference* emitter, MpActor* listener) {
              return pImpl->onSubscribe(sendTarget, emitter, listener);
            },
            [sendTarget, this](MpObjectReference* emitter, MpActor* listener) {
              return pImpl->onUnsubscribe(sendTarget, emitter, listener);
            },
            baseId, typeStr.data())),
          formId, true);
      }
    }
  }

  printf("AttachEspm took %d ticks\n", int(clock() - was));
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

void PartOne::AddUser(Networking::UserId userId, UserType type)
{
  serverState.Connect(userId);
  for (auto& listener : pImpl->listeners)
    listener->OnConnect(userId);
}

MpActor* PartOne::SendToNeighbours(const simdjson::dom::element& jMessage,
                                   Networking::UserId userId,
                                   Networking::PacketData data, size_t length,
                                   bool reliable)
{
  int64_t idx;
  Read(jMessage, "idx", &idx);

  MpActor* actor = serverState.ActorByUser(userId);

  if (actor) {
    if (idx != actor->GetIdx()) {
      std::stringstream ss;
      ss << std::hex << "You aren't able to update actor with idx " << idx
         << " (your actor's idx is " << actor->GetIdx() << ')';
      throw PublicError(ss.str());
    }

    for (auto listener : actor->GetListeners()) {
      auto targetuserId = serverState.UserByActor(listener);
      if (targetuserId != Networking::InvalidUserId)
        pushedSendTarget->Send(targetuserId, data, length, reliable);
    }
  }

  return actor;
}

void PartOne::HandleMessagePacket(Networking::UserId userId,
                                  Networking::PacketData data, size_t length)
{
  if (!serverState.IsConnected(userId))
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");
  auto& user = *serverState.userInfo[userId];

  if (!length)
    throw std::runtime_error("Zero-length message packets are not allowed");
  auto jMessage = pImpl->parser.parse(data + 1, length - 1).value();

  using TypeInt = std::underlying_type<MsgType>::type;
  auto type = MsgType::Invalid;
  Read(jMessage, "t", reinterpret_cast<TypeInt*>(&type));

  switch (type) {
    case MsgType::CustomPacket: {
      simdjson::dom::element content;
      Read(jMessage, "content", &content);
      for (auto& listener : pImpl->listeners) {
        listener->OnCustomPacket(userId, content);
      }
      break;
    }
    case MsgType::UpdateAnimation: {
      SendToNeighbours(jMessage, userId, data, length);
      break;
    }

    case MsgType::UpdateLook: {
      simdjson::dom::element jData;
      Read(jMessage, "data", &jData);

      auto look = MpActor::Look::FromJson(jData);
      // TODO: validate

      auto actor = SendToNeighbours(jMessage, userId, data, length, true);

      if (actor) {
        actor->SetLook(&look);
      }
      break;
    }
    case MsgType::UpdateMovement: {
      simdjson::dom::element data_;
      Read(jMessage, "data", &data_);

      auto actor = SendToNeighbours(jMessage, userId, data, length);

      if (actor) {
        simdjson::dom::element jPos;
        Read(data_, "pos", &jPos);
        double pos[3];
        for (int i = 0; i < 3; ++i)
          Read(jPos, i, &pos[i]);
        actor->SetPos({ (float)pos[0], (float)pos[1], (float)pos[2] });
      }
      if (actor) {
        simdjson::dom::element jRot;
        Read(data_, "rot", &jRot);
        double rot[3];
        for (int i = 0; i < 3; ++i)
          Read(jRot, i, &rot[i]);
        actor->SetAngle({ (float)rot[0], (float)rot[1], (float)rot[2] });
      }
      break;
    }
    case MsgType::UpdateEquipment: {

      MpActor* actor = serverState.ActorByUser(userId);
      if (actor) {
        simdjson::dom::element data_;
        ReadEx(jMessage, "data", &data_);
        simdjson::dom::element inv;
        ReadEx(data_, "inv", &inv);

        auto equipmentInv = Inventory::FromJson(inv);

        bool badEq = false;
        for (auto& e : equipmentInv.entries) {
          if (!actor->GetInventory().HasItem(e.baseId)) {
            badEq = true;
            break;
          }
        }

        if (!badEq) {
          SendToNeighbours(jMessage, userId, data, length, true);

          simdjson::dom::element data_;
          Read(jMessage, "data", &data_);
          actor->SetEquipment(simdjson::minify(data_));
        }
      }
      break;
    }
    case MsgType::Activate: {
      simdjson::dom::element data_;
      ReadEx(jMessage, "data", &data_);
      uint32_t caster, target;
      ReadEx(data_, "caster", &caster);
      ReadEx(data_, "target", &target);

      HandleActivate(userId, caster, target);

      break;
    }
    case MsgType::PutItem:
    case MsgType::TakeItem: {
      MpActor* actor = serverState.ActorByUser(userId);
      if (actor && pImpl->espm) {
        uint32_t target;
        ReadEx(jMessage, "target", &target);
        auto e = Inventory::Entry::FromJson(jMessage);
        auto& ref = worldState.GetFormAt<MpObjectReference>(target);

        if (type == MsgType::PutItem)
          ref.PutItem(*actor, e);
        else
          ref.TakeItem(*actor, e);
      }
      break;
    }
    default:
      throw PublicError("Unknown MsgType: " + std::to_string((TypeInt)type));
  }
}

void PartOne::HandleActivate(Networking::UserId userId, uint32_t caster,
                             uint32_t target)
{
  if (!pImpl->espm)
    throw std::runtime_error("No loaded esm or esp files are found");

  const auto ac = serverState.ActorByUser(userId);
  if (!ac)
    throw std::runtime_error("Can't do this without Actor attached");

  if (caster != 0x14) {
    std::stringstream ss;
    ss << "Bad caster (0x" << std::hex << caster << ")";
    throw std::runtime_error(ss.str());
  }

  auto refPtr = std::dynamic_pointer_cast<MpObjectReference>(
    worldState.LookupFormById(target));
  if (!refPtr)
    return;

  refPtr->Activate(*ac);
}