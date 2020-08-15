#include "MpObjectReference.h"
#include "MpActor.h"
#include "WorldState.h"

namespace {
std::pair<int16_t, int16_t> GetGridPos(const NiPoint3& pos) noexcept
{
  return { int16_t(pos.x / 4096), int16_t(pos.y / 4096) };
}
}

void MpObjectReference::SetPos(const NiPoint3& newPos)
{
  auto& grid = GetParent()->grids[cellOrWorld];

  auto oldGridPos = GetGridPos(pos);
  auto newGridPos = GetGridPos(newPos);
  if (oldGridPos != newGridPos || !isOnGrid) {
    InitListenersAndEmitters();

    grid.Move(this, newGridPos.first, newGridPos.second);
    isOnGrid = true;

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
  pos = newPos;
}

void MpObjectReference::SetAngle(const NiPoint3& newAngle)
{
  rot = newAngle;
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

void MpObjectReference::InitListenersAndEmitters()
{
  if (!listeners) {
    listeners.reset(new std::set<MpActor*>);
    emitters.reset(new std::set<MpObjectReference*>);
  }
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