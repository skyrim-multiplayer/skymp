#pragma once
#include "Grid.h"
#include "GridElement.h"
#include "NiPoint3.h"
#include <algorithm>
#include <sparsepp/spp.h>
#include <sstream>
#include <typeinfo>

#ifdef AddForm
#  undef AddForm
#endif

class MpForm
{
  friend class WorldState;

public:
  virtual ~MpForm() = default;

  auto GetFormId() const noexcept { return formId; }

protected:
  auto GetParent() const { return parent; }

private:
  // Assigned by WorldState::AddForm
  uint32_t formId = 0;
  WorldState* parent = nullptr;
};

struct LocationalData
{
  NiPoint3 pos, rot;
  uint32_t cellOrWorld;
};

class MpActor
  : public MpForm
  , private LocationalData
{
public:
  using SubscribeCallback =
    std::function<void(MpActor* emitter, MpActor* listener)>;

  MpActor(const LocationalData& locationalData_,
          const SubscribeCallback& onSubscribe_,
          const SubscribeCallback& onUnsubscribe_)
    : onSubscribe(onSubscribe_)
    , onUnsubscribe(onUnsubscribe_)
  {
    static_cast<LocationalData&>(*this) = locationalData_;
  }

  ~MpActor();

  const auto& GetPos() { return pos; }
  const auto& GetAngle() { return rot; }
  const auto& GetCellOrWorld() { return cellOrWorld; }

  void SetPos(const NiPoint3& newPos);

  void SetAngle(const NiPoint3& newAngle);

  static void Subscribe(MpActor* emitter, MpActor* listener)
  {
    emitter->listeners.insert(listener);
    listener->emitters.insert(emitter);
    emitter->onSubscribe(emitter, listener);
  }

  static void Unsubscribe(MpActor* emitter, MpActor* listener)
  {
    emitter->onUnsubscribe(emitter, listener);
    emitter->listeners.erase(listener);
    listener->emitters.erase(emitter);
  }

  auto& GetListeners() const { return listeners; }

private:
  bool isOnGrid = false;
  std::set<MpActor*> listeners;
  std::set<MpActor*> emitters;

  const SubscribeCallback onSubscribe, onUnsubscribe;
};

class WorldState
{
  friend class MpActor;

public:
  void AddForm(std::unique_ptr<MpForm> form, uint32_t formId)
  {
    auto& f = forms[formId];
    if (f) {
      std::stringstream ss;
      ss << "Form with id " << std::hex << formId << " already exists";
      throw std::runtime_error(ss.str());
    }
    form->formId = formId;
    form->parent = this;
    f = std::move(form);
  }

  template <class FormType = MpForm>
  void DestroyForm(uint32_t formId,
                   std::shared_ptr<FormType>* outDestroyedForm = nullptr)
  {
    auto it = forms.find(formId);
    if (it == forms.end()) {
      std::stringstream ss;
      ss << "Form with id " << std::hex << formId << "doesn't exist";
      throw std::runtime_error(ss.str());
    }

    auto& [formId_, form] = *it;
    if (!dynamic_cast<FormType*>(form.get())) {
      std::stringstream ss;
      ss << "Expected form " << std::hex << formId << " to be "
         << typeid(FormType).name() << ", but got "
         << typeid(*form.get()).name();
      throw std::runtime_error(ss.str());
    }

    if (outDestroyedForm)
      *outDestroyedForm = std::dynamic_pointer_cast<FormType>(it->second);
    forms.erase(it);
  }

  const std::shared_ptr<MpForm>& LookupFormById(uint32_t formId)
  {
    auto it = forms.find(formId);
    if (it == forms.end()) {
      static const std::shared_ptr<MpForm> g_null;
      return g_null;
    }
    return it->second;
  }

  template <class F>
  void ForEachNeighbour(uint32_t worldOrCell, int16_t gridX, int16_t gridY,
                        F f)
  {
    auto& neighbours = grids[worldOrCell].GetNeighbours(gridX, gridY);
    for (auto nei : neighbours)
      f(nei);
  }

private:
  spp::sparse_hash_map<uint32_t, std::shared_ptr<MpForm>> forms;
  spp::sparse_hash_map<uint32_t, Grid<MpActor*>> grids;
};

inline std::pair<int16_t, int16_t> GetGridPos(const NiPoint3& pos) noexcept
{
  return { int16_t(pos.x / 4096), int16_t(pos.y / 4096) };
}

inline MpActor::~MpActor()
{
  GetParent()->grids[cellOrWorld].Forget(this);

  auto listenersCopy = listeners;
  for (auto listener : listenersCopy)
    Unsubscribe(this, listener);

  auto emittersCopy = emitters;
  for (auto emitter : emittersCopy)
    Unsubscribe(emitter, this);
}

inline void MpActor::SetPos(const NiPoint3& newPos)
{
  auto& grid = GetParent()->grids[cellOrWorld];

  auto oldGridPos = GetGridPos(pos);
  auto newGridPos = GetGridPos(newPos);
  if (oldGridPos != newGridPos || !isOnGrid) {
    grid.Move(this, newGridPos.first, newGridPos.second);
    isOnGrid = true;

    auto& was = this->listeners;
    auto& now = grid.GetNeighboursAndMe(this);

    std::vector<MpActor*> toRemove;
    std::set_difference(was.begin(), was.end(), now.begin(), now.end(),
                        std::inserter(toRemove, toRemove.begin()));
    for (auto listener : toRemove) {
      Unsubscribe(this, listener);
      Unsubscribe(listener, this);
    }

    std::vector<MpActor*> toAdd;
    std::set_difference(now.begin(), now.end(), was.begin(), was.end(),
                        std::inserter(toAdd, toAdd.begin()));
    for (auto listener : toAdd) {
      Subscribe(this, listener);
      Subscribe(listener, this);
    }
  }
  pos = newPos;
}

inline void MpActor::SetAngle(const NiPoint3& newAngle)
{
  rot = newAngle;
}