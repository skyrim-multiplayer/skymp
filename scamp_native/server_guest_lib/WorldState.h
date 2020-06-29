#pragma once
#include "Grid.h"
#include "GridElement.h"
#include "NiPoint3.h"
#include <MakeID.h>
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
  virtual void BeforeDestroy(){};

  // Assigned by WorldState::AddForm
  uint32_t formId = 0;
  WorldState* parent = nullptr;
};

struct LocationalData
{
  NiPoint3 pos, rot;
  uint32_t cellOrWorld;
};

class FormIndex
{
public:
  constexpr static uint32_t g_invalidIdx = (uint32_t)-1;

  const auto& GetIdx() { return idx; }

  uint32_t idx = g_invalidIdx;
};

class MpActor
  : public MpForm
  , private LocationalData
  , public FormIndex
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

  ~MpActor() = default;

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
  void BeforeDestroy() override;

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
      throw std::runtime_error((std::stringstream()
                                << "Form with id " << std::hex << formId
                                << " already exists")
                                 .str());
    }
    form->formId = formId;
    form->parent = this;

    if (auto formIndex = dynamic_cast<FormIndex*>(form.get())) {
      if (!formIdxManager)
        formIdxManager.reset(new MakeID(FormIndex::g_invalidIdx - 1));
      if (!formIdxManager->CreateID(formIndex->idx))
        throw std::runtime_error("CreateID failed");
    }

    f = std::move(form);
  }

  template <class FormType = MpForm>
  void DestroyForm(uint32_t formId,
                   std::shared_ptr<FormType>* outDestroyedForm = nullptr)
  {
    auto it = forms.find(formId);
    if (it == forms.end()) {
      throw std::runtime_error((std::stringstream()
                                << "Form with id " << std::hex << formId
                                << " doesn't exist")
                                 .str());
    }

    auto& [formId_, form] = *it;
    if (!dynamic_cast<FormType*>(form.get())) {
      throw std::runtime_error((std::stringstream()
                                << "Expected form " << std::hex << formId
                                << " to be " << typeid(FormType).name()
                                << ", but got " << typeid(*form.get()).name())
                                 .str());
    }

    if (outDestroyedForm)
      *outDestroyedForm = std::dynamic_pointer_cast<FormType>(it->second);

    it->second->BeforeDestroy();

    if (auto formIndex = dynamic_cast<FormIndex*>(form.get())) {
      if (formIdxManager && !formIdxManager->DestroyID(formIndex->idx))
        throw std::runtime_error("DestroyID failed");
    }

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

private:
  spp::sparse_hash_map<uint32_t, std::shared_ptr<MpForm>> forms;
  spp::sparse_hash_map<uint32_t, Grid<MpActor*>> grids;
  std::unique_ptr<MakeID> formIdxManager;
};

inline std::pair<int16_t, int16_t> GetGridPos(const NiPoint3& pos) noexcept
{
  return { int16_t(pos.x / 4096), int16_t(pos.y / 4096) };
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
      if (listener != this)
        Unsubscribe(listener, this);
    }

    std::vector<MpActor*> toAdd;
    std::set_difference(now.begin(), now.end(), was.begin(), was.end(),
                        std::inserter(toAdd, toAdd.begin()));
    for (auto listener : toAdd) {
      Subscribe(this, listener);
      if (listener != this)
        Subscribe(listener, this);
    }
  }
  pos = newPos;
}

inline void MpActor::SetAngle(const NiPoint3& newAngle)
{
  rot = newAngle;
}

inline void MpActor::BeforeDestroy()
{
  GetParent()->grids[cellOrWorld].Forget(this);

  auto listenersCopy = listeners;
  for (auto listener : listenersCopy)
    Unsubscribe(this, listener);

  auto emittersCopy = emitters;
  for (auto emitter : emittersCopy)
    Unsubscribe(emitter, this);
}