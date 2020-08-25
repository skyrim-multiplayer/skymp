#include "MpObjectReference.h"
#include "LeveledListUtils.h"
#include "MpActor.h"
#include "WorldState.h"
#include <MsgType.h>

class OccupantDestroyEventSink : public MpActor::DestroyEventSink
{
public:
  OccupantDestroyEventSink(WorldState& wst_,
                           MpObjectReference* untrustedRefPtr_)
    : wst(wst_)
    , untrustedRefPtr(untrustedRefPtr_)
    , refId(untrustedRefPtr_->GetFormId())
  {
  }

  void BeforeDestroy(MpActor& actor) override
  {
    if (!RefStillValid())
      return;
    if (untrustedRefPtr->occupant == &actor) {
      untrustedRefPtr->SetOpen(false);
      untrustedRefPtr->occupant = nullptr;
    }
  }

private:
  bool RefStillValid() const
  {
    return untrustedRefPtr == wst.LookupFormById(refId).get();
  }

  WorldState& wst;
  MpObjectReference* const untrustedRefPtr;
  const uint32_t refId;
};

namespace {
std::pair<int16_t, int16_t> GetGridPos(const NiPoint3& pos) noexcept
{
  return { int16_t(pos.x / 4096), int16_t(pos.y / 4096) };
}
}

MpObjectReference::MpObjectReference(const LocationalData& locationalData_,
                                     const SubscribeCallback& onSubscribe_,
                                     const SubscribeCallback& onUnsubscribe_,
                                     uint32_t baseId_, const char* baseType_)
  : onSubscribe(onSubscribe_)
  , onUnsubscribe(onUnsubscribe_)
  , baseId(baseId_)
  , baseType(baseType_)
{
  static_cast<LocationalData&>(*this) = locationalData_;

  if (!strcmp(baseType_, "FLOR") || !strcmp(baseType_, "TREE")) {
    relootTime = std::chrono::hours(1);
  } else if (!strcmp(baseType_, "DOOR")) {
    relootTime = std::chrono::seconds(3);
  } else if (espm::IsItem(baseType_)) {
    relootTime = std::chrono::hours(1);
  }
}

void MpObjectReference::VisitProperties(const PropertiesVisitor& visitor)
{
  if (IsHarvested())
    visitor("isHarvested", "true");
  if (IsOpen())
    visitor("isOpen", "true");
}

void MpObjectReference::SetPos(const NiPoint3& newPos)
{
  auto& grid = GetParent()->grids[cellOrWorld];

  auto oldGridPos = GetGridPos(pos);
  auto newGridPos = GetGridPos(newPos);
  pos = newPos;
  if (oldGridPos != newGridPos || !everSubscribedOrListened) {
    everSubscribedOrListened = true;

    InitListenersAndEmitters();

    MoveOnGrid(grid);

    auto& was = *this->listeners;
    auto& now = grid.GetNeighboursAndMe(this);

    std::vector<MpObjectReference*> toRemove;
    std::set_difference(was.begin(), was.end(), now.begin(), now.end(),
                        std::inserter(toRemove, toRemove.begin()));
    for (auto listener : toRemove) {
      Unsubscribe(this, listener);
      if (this != listener)
        Unsubscribe(listener, this);
    }

    std::vector<MpObjectReference*> toAdd;
    std::set_difference(now.begin(), now.end(), was.begin(), was.end(),
                        std::inserter(toAdd, toAdd.begin()));
    for (auto listener : toAdd) {
      Subscribe(this, listener);
      if (this != listener)
        Subscribe(listener, this);
    }
  }
}

void MpObjectReference::SetAngle(const NiPoint3& newAngle)
{
  rot = newAngle;
}

void MpObjectReference::SetHarvested(bool harvested)
{
  if (harvested != isHarvested) {
    isHarvested = harvested;
    SendPropertyToListeners("isHarvested", harvested);
  }
}

void MpObjectReference::SetOpen(bool open)
{
  if (open != isOpen) {
    isOpen = open;
    SendPropertyToListeners("isOpen", open);
  }
}

void MpObjectReference::Activate(MpActor& activationSource)
{
  std::cout << "Activate " << this->GetFormId() << " by "
            << activationSource.GetFormId() << std::endl;

  auto& loader = GetParent()->GetEspm();
  auto& compressedFieldsCache = GetParent()->GetEspmCache();

  CheckInteractionAbility(activationSource);

  auto base = loader.GetBrowser().LookupById(GetBaseId());
  if (!base.rec || !GetBaseId()) {
    std::stringstream ss;
    ss << std::hex << GetFormId() << " doesn't have base form";
    throw std::runtime_error(ss.str());
  }

  auto t = base.rec->GetType();

  if (t == espm::TREE::type || t == espm::FLOR::type || espm::IsItem(t)) {
    if (!IsHarvested()) {
      auto mapping = loader.GetBrowser().GetMapping(base.fileIdx);
      uint32_t resultItem = 0;
      if (t == espm::TREE::type) {
        espm::FLOR::Data data;
        data = espm::Convert<espm::TREE>(base.rec)->GetData();
        resultItem = espm::GetMappedId(data.resultItem, *mapping);
      } else if (t == espm::FLOR::type) {
        espm::FLOR::Data data;
        data = espm::Convert<espm::FLOR>(base.rec)->GetData();
        resultItem = espm::GetMappedId(data.resultItem, *mapping);
      } else {
        resultItem = espm::GetMappedId(base.rec->GetId(), *mapping);
      }

      activationSource.AddItem(resultItem, 1);
      SetHarvested(true);
      RequestReloot();
    }
  } else if (t == espm::DOOR::type) {

    auto refrRecord = espm::Convert<espm::REFR>(
      loader.GetBrowser().LookupById(GetFormId()).rec);
    auto teleport = refrRecord->GetData().teleport;
    if (teleport) {
      if (!IsOpen()) {
        SetOpen(true);
        RequestReloot();
      }

      auto destinationRecord = espm::Convert<espm::REFR>(
        loader.GetBrowser().LookupById(teleport->destinationDoor).rec);
      if (!destinationRecord)
        throw std::runtime_error(
          "No destination found for this teleport door");

      auto teleportWorldOrCell = espm::GetWorldOrCell(destinationRecord);

      static const auto g_pi = std::acos(-1.f);
      std::string msg;
      msg += Networking::MinPacketId;
      msg += nlohmann::json{
        { "pos", { teleport->pos[0], teleport->pos[1], teleport->pos[2] } },
        { "rot",
          { teleport->rotRadians[0] / g_pi * 180,
            teleport->rotRadians[1] / g_pi * 180,
            teleport->rotRadians[2] / g_pi * 180 } },
        { "worldOrCell", teleportWorldOrCell },
        { "type", "teleport" }
      }.dump();
      activationSource.SendToUser(msg.data(), msg.size(), true);

      activationSource.SetCellOrWorld(teleportWorldOrCell);

    } else {
      SetOpen(!IsOpen());
    }
  } else if (t == espm::CONT::type) {
    EnsureBaseContainerAdded(loader);
    if (!this->occupant) {
      SetOpen(true);
      SendPropertyTo("inventory", GetInventory().ToJson(), activationSource);
      activationSource.SendOpenContainer(GetFormId());

      this->occupant = &activationSource;

      this->occupantDestroySink.reset(
        new OccupantDestroyEventSink(*GetParent(), this));
      this->occupant->AddEventSink(occupantDestroySink);
    } else if (this->occupant == &activationSource) {
      SetOpen(false);
      this->occupant->RemoveEventSink(this->occupantDestroySink);
      this->occupant = nullptr;
    }
  }
}

void MpObjectReference::PutItem(MpActor& ac, const Inventory::Entry& e)
{
  std::cout << "PutItem into " << this->GetFormId() << " by " << ac.GetFormId()
            << std::endl;

  CheckInteractionAbility(ac);
  if (this->occupant != &ac) {
    std::stringstream err;
    err << std::hex << "Actor 0x" << ac.GetFormId() << " doesn't occupy ref 0x"
        << GetFormId();
    throw std::runtime_error(err.str());
  }
  ac.RemoveItems({ e }, this);
}

void MpObjectReference::TakeItem(MpActor& ac, const Inventory::Entry& e)
{
  std::cout << "TakeItem from " << this->GetFormId() << " by "
            << ac.GetFormId() << std::endl;

  CheckInteractionAbility(ac);
  if (this->occupant != &ac) {
    std::stringstream err;
    err << std::hex << "Actor 0x" << ac.GetFormId() << " doesn't occupy ref 0x"
        << GetFormId();
    throw std::runtime_error(err.str());
  }
  RemoveItems({ e }, &ac);
}

void MpObjectReference::SetRelootTime(std::chrono::milliseconds newRelootTime)
{
  relootTime = newRelootTime;
}

void MpObjectReference::SetCellOrWorld(uint32_t newWorldOrCell)
{
  everSubscribedOrListened = false;
  auto& grid = GetParent()->grids[cellOrWorld];
  grid.Forget(this);

  cellOrWorld = newWorldOrCell;
}

void MpObjectReference::SetChanceNoneOverride(uint8_t newChanceNone)
{
  chanceNoneOverride.reset(new uint8_t(newChanceNone));
}

void MpObjectReference::AddItem(uint32_t baseId, uint32_t count)
{
  inv.AddItem(baseId, count);
  SendInventoryUpdate();
}

void MpObjectReference::AddItems(const std::vector<Inventory::Entry>& entries)
{
  if (entries.size() > 0) {
    inv.AddItems(entries);
    SendInventoryUpdate();
  }
}

void MpObjectReference::RemoveItems(
  const std::vector<Inventory::Entry>& entries, MpObjectReference* target)
{
  inv.RemoveItems(entries);
  if (target)
    target->AddItems(entries);

  SendInventoryUpdate();
}

void MpObjectReference::Subscribe(MpObjectReference* emitter,
                                  MpObjectReference* listener)
{
  bool bothNonActors =
    !dynamic_cast<MpActor*>(emitter) && !dynamic_cast<MpActor*>(listener);
  if (bothNonActors)
    return;

  emitter->InitListenersAndEmitters();
  listener->InitListenersAndEmitters();
  emitter->listeners->insert(listener);
  listener->emitters->insert(emitter);
  emitter->onSubscribe(emitter, listener);
}

void MpObjectReference::Unsubscribe(MpObjectReference* emitter,
                                    MpObjectReference* listener)
{
  bool bothNonActors =
    !dynamic_cast<MpActor*>(emitter) && !dynamic_cast<MpActor*>(listener);
  if (bothNonActors)
    return;

  emitter->onUnsubscribe(emitter, listener);
  emitter->listeners->erase(listener);
  listener->emitters->erase(emitter);
}

const std::set<MpObjectReference*>& MpObjectReference::GetListeners() const
{
  static const std::set<MpObjectReference*> g_emptyListeners;
  return listeners ? *listeners : g_emptyListeners;
}

const std::set<MpObjectReference*>& MpObjectReference::GetEmitters() const
{
  static const std::set<MpObjectReference*> g_emptyEmitters;
  return emitters ? *emitters : g_emptyEmitters;
}

void MpObjectReference::Init(WorldState* parent, uint32_t formId)
{
  MpForm::Init(parent, formId);

  auto& grid = GetParent()->grids[cellOrWorld];
  MoveOnGrid(grid);
}

void MpObjectReference::MoveOnGrid(GridImpl<MpObjectReference*>& grid)
{
  auto newGridPos = GetGridPos(pos);
  grid.Move(this, newGridPos.first, newGridPos.second);
}

void MpObjectReference::InitListenersAndEmitters()
{
  if (!listeners) {
    listeners.reset(new std::set<MpObjectReference*>);
    emitters.reset(new std::set<MpObjectReference*>);
  }
}

void MpObjectReference::RequestReloot()
{
  GetParent()->RequestReloot(*this);
}

void MpObjectReference::SendInventoryUpdate()
{
  auto actor = dynamic_cast<MpActor*>(this);
  if (actor) {
    std::string msg;
    msg += Networking::MinPacketId;
    msg += nlohmann::json{
      { "inventory", actor->GetInventory().ToJson() },
      { "type", "setInventory" }
    }.dump();
    actor->SendToUser(msg.data(), msg.size(), true);
  }
}

void MpObjectReference::SendOpenContainer(uint32_t targetId)
{
  auto actor = dynamic_cast<MpActor*>(this);
  if (actor) {
    std::string msg;
    msg += Networking::MinPacketId;
    msg += nlohmann::json{
      { "target", targetId }, { "type", "openContainer" }
    }.dump();
    actor->SendToUser(msg.data(), msg.size(), true);
  }
}

void MpObjectReference::EnsureBaseContainerAdded(espm::Loader& espm)
{
  if (this->baseContainerAdded)
    return;

  constexpr uint32_t pcLevel = 1;

  std::map<uint32_t, uint32_t> itemsToAdd;

  auto lookupRes = espm.GetBrowser().LookupById(GetBaseId());
  auto baseContainer = espm::Convert<espm::CONT>(lookupRes.rec);
  if (baseContainer) {
    auto data = baseContainer->GetData();
    for (auto& entry : data.objects) {
      auto formLookupRes = espm.GetBrowser().LookupById(entry.formId);
      auto leveledItem = espm::Convert<espm::LVLI>(formLookupRes.rec);
      if (leveledItem) {
        auto map = LeveledListUtils::EvaluateListRecurse(
          espm.GetBrowser(), formLookupRes, 1, pcLevel,
          chanceNoneOverride.get());
        for (auto& p : map)
          itemsToAdd[p.first] += p.second;
      } else
        itemsToAdd[entry.formId] += entry.count;
    }
  }

  std::vector<Inventory::Entry> entries;
  for (auto& p : itemsToAdd)
    entries.push_back({ p.first, p.second });
  AddItems(entries);

  this->baseContainerAdded = true;
}

void MpObjectReference::CheckInteractionAbility(MpActor& ac)
{
  auto& loader = GetParent()->GetEspm();
  auto& compressedFieldsCache = GetParent()->GetEspmCache();

  auto casterWorld = loader.GetBrowser().LookupById(ac.GetCellOrWorld()).rec;
  auto targetWorld = loader.GetBrowser().LookupById(GetCellOrWorld()).rec;

  if (targetWorld != casterWorld) {
    const char* casterWorldName =
      casterWorld ? casterWorld->GetEditorId(&compressedFieldsCache) : "";

    const char* targetWorldName =
      targetWorld ? targetWorld->GetEditorId(&compressedFieldsCache) : "";
    std::stringstream ss;
    ss << "WorldSpace doesn't match: caster is in " << casterWorldName
       << ", target is in " << targetWorldName;
    throw std::runtime_error(ss.str());
  }
}

namespace {
std::string CreatePropertyMessage(MpObjectReference* self, const char* name,
                                  const nlohmann::json& value)
{
  nlohmann::json j{ { "idx", self->GetIdx() },
                    { "t", MsgType::UpdateProperty },
                    { "propName", name },
                    { "data", value } };
  std::string str;
  str += Networking::MinPacketId;
  str += j.dump();
  return str;
}
}

void MpObjectReference::SendPropertyToListeners(const char* name,
                                                const nlohmann::json& value)
{
  auto str = CreatePropertyMessage(this, name, value);
  for (auto listener : GetListeners()) {
    auto listenerAsActor = dynamic_cast<MpActor*>(listener);
    if (listenerAsActor)
      listenerAsActor->SendToUser(str.data(), str.size(), true);
  }
}

void MpObjectReference::SendPropertyTo(const char* name,
                                       const nlohmann::json& value,
                                       MpActor& target)
{
  auto str = CreatePropertyMessage(this, name, value);
  target.SendToUser(str.data(), str.size(), true);
}

void MpObjectReference::BeforeDestroy()
{
  if (this->occupant && this->occupantDestroySink)
    this->occupant->RemoveEventSink(this->occupantDestroySink);

  MpForm::BeforeDestroy();

  GetParent()->grids[cellOrWorld].Forget(this);

  auto listenersCopy = GetListeners();
  for (auto listener : listenersCopy)
    if (this != listener)
      Unsubscribe(this, listener);
}