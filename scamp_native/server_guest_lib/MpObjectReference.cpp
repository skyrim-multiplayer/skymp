#include "MpObjectReference.h"
#include "MpActor.h"
#include "WorldState.h"
#include <MsgType.h>

namespace {
std::pair<int16_t, int16_t> GetGridPos(const NiPoint3& pos) noexcept
{
  return { int16_t(pos.x / 4096), int16_t(pos.y / 4096) };
}
}

MpObjectReference::MpObjectReference(const LocationalData& locationalData_,
                                     const SubscribeCallback& onSubscribe_,
                                     const SubscribeCallback& onUnsubscribe_,
                                     uint32_t baseId_)
  : onSubscribe(onSubscribe_)
  , onUnsubscribe(onUnsubscribe_)
  , baseId(baseId_)
{
  static_cast<LocationalData&>(*this) = locationalData_;
}

void MpObjectReference::VisitProperties(const PropertiesVisitor& visitor)
{
  if (IsHarvested())
    visitor("isHarvested", "true");
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

    auto thisAsActor = dynamic_cast<MpActor*>(this);

    for (auto listener : toRemove) {
      auto listenerAsActor = dynamic_cast<MpActor*>(listener);
      if (listenerAsActor)
        Unsubscribe(this, listenerAsActor);

      if (thisAsActor && listener != this)
        Unsubscribe(listener, thisAsActor);
    }

    std::vector<MpObjectReference*> toAdd;
    std::set_difference(now.begin(), now.end(), was.begin(), was.end(),
                        std::inserter(toAdd, toAdd.begin()));
    for (auto listener : toAdd) {
      auto listenerAsActor = dynamic_cast<MpActor*>(listener);
      if (listenerAsActor)
        Subscribe(this, listenerAsActor);
      if (thisAsActor && listener != this)
        Subscribe(listener, thisAsActor);
    }
  }
}

void MpObjectReference::SetAngle(const NiPoint3& newAngle)
{
  rot = newAngle;
}

void MpObjectReference::SetHarvested(bool harvested)
{
  if (harvested == isHarvested)
    return;
  isHarvested = harvested;

  nlohmann::json j{ { "idx", GetIdx() },
                    { "t", MsgType::UpdateProperty },
                    { "propName", "isHarvested" },
                    { "data", harvested } };
  std::string str;
  str += Networking::MinPacketId;
  str += j.dump();

  for (auto listener : GetListeners())
    listener->SendToUser(str.data(), str.size(), true);
}

void MpObjectReference::Activate(
  MpActor& activationSource, espm::Loader& loader,
  espm::CompressedFieldsCache& compressedFieldsCache)
{
  auto casterWorld =
    loader.GetBrowser().LookupById(activationSource.GetCellOrWorld()).rec;
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

  auto base = loader.GetBrowser().LookupById(GetBaseId());
  if (!base.rec || !GetBaseId()) {
    std::stringstream ss;
    ss << std::hex << GetFormId() << " doesn't have base form";
    throw std::runtime_error(ss.str());
  }

  auto t = base.rec->GetType();

  if (t == espm::TREE::type || t == espm::FLOR::type) {
    espm::FLOR::Data data;
    if (t == espm::TREE::type)
      data = espm::Convert<espm::TREE>(base.rec)->GetData();
    else
      data = espm::Convert<espm::FLOR>(base.rec)->GetData();

    activationSource.AddItem(data.resultItem, 1);
    SetHarvested(true);
    RequestReloot();
  }
}

void MpObjectReference::SetRelootTime(std::chrono::milliseconds newRelootTime)
{
  relootTime = newRelootTime;
}

void MpObjectReference::AddItem(uint32_t baseId, uint32_t count)
{
  inv.AddItem(baseId, count);

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

void MpObjectReference::Subscribe(MpObjectReference* emitter,
                                  MpActor* listener)
{
  emitter->InitListenersAndEmitters();
  listener->InitListenersAndEmitters();
  emitter->listeners->insert(listener);
  listener->emitters->insert(emitter);
  emitter->onSubscribe(emitter, listener);
}

void MpObjectReference::Unsubscribe(MpObjectReference* emitter,
                                    MpActor* listener)
{
  emitter->onUnsubscribe(emitter, listener);
  emitter->listeners->erase(listener);
  listener->emitters->erase(emitter);
}

const std::set<MpActor*>& MpObjectReference::GetListeners() const
{
  static const std::set<MpActor*> g_emptyListeners;
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
    listeners.reset(new std::set<MpActor*>);
    emitters.reset(new std::set<MpObjectReference*>);
  }
}

void MpObjectReference::RequestReloot()
{
  GetParent()->RequestReloot(*this);
}

void MpObjectReference::BeforeDestroy()
{
  MpForm::BeforeDestroy();

  GetParent()->grids[cellOrWorld].Forget(this);

  auto listenersCopy = GetListeners();
  for (auto listener : listenersCopy)
    if (this != listener)
      Unsubscribe(this, listener);
}