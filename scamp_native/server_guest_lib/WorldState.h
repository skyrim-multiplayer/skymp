#pragma once
#include "Grid.h"
#include "GridElement.h"
#include "JsonUtils.h"
#include "NiPoint3.h"
#include <MakeID.h>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <sparsepp/spp.h>
#include <sstream>
#include <typeinfo>

#ifdef AddForm
#  undef AddForm
#endif

class WorldState;

class MpForm
{
  friend class WorldState;

public:
  static const char* PrettifyType(const char* typeidName)
  {
#ifdef WIN32
    return typeidName + strlen("class Mp");
#else
    auto name = typeidName;
    while (memcmp(name, "Mp", 2) != 0)
      ++name;
    return name + 2;
#endif
  }

  template <class F>
  static const char* GetFormType()
  {
    return PrettifyType(typeid(F).name());
  }

  static const char* GetFormType(MpForm* form)
  {
    return PrettifyType(typeid(form).name());
  }

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
  struct Tint
  {
    std::string texturePath;
    int32_t argb = 0;
    int32_t type = 0;

    static Tint FromJson(simdjson::dom::element& j)
    {
      Tint res;
      ReadEx(j, "argb", &res.argb);
      ReadEx(j, "type", &res.type);

      const char* texturePathCstr = "";
      ReadEx(j, "texturePath", &texturePathCstr);
      res.texturePath = texturePathCstr;

      return res;
    }
  };

  struct Look
  {
    bool isFemale = false;
    uint32_t raceId = 0;
    float weight = 0.f;
    int32_t skinColor = 0;
    int32_t hairColor = 0;
    std::vector<uint32_t> headpartIds;
    uint32_t headTextureSetId = 0;
    std::vector<float> faceMorphs;
    std::vector<float> facePresets;
    std::vector<Tint> tints;

    static Look FromJson(const nlohmann::json& j)
    {
      simdjson::dom::parser p;
      simdjson::dom::element parsed = p.parse(j.dump());
      return FromJson(parsed);
    }

    static Look FromJson(simdjson::dom::element& j)
    {
      Look res;
      ReadEx(j, "isFemale", &res.isFemale);
      ReadEx(j, "raceId", &res.raceId);
      ReadEx(j, "weight", &res.weight);
      ReadEx(j, "skinColor", &res.skinColor);
      ReadEx(j, "hairColor", &res.hairColor);
      ReadVector(j, "headpartIds", &res.headpartIds);
      ReadEx(j, "headTextureSetId", &res.headTextureSetId);
      ReadVector(j, "options", &res.faceMorphs);
      ReadVector(j, "presets", &res.facePresets);

      simdjson::dom::element jTints;
      ReadEx(j, "tints", &jTints);

      res.tints.reserve(30);
      auto jTintsArr = jTints.operator simdjson::dom::array();
      for (simdjson::dom::element el : jTintsArr) {
        res.tints.push_back(Tint::FromJson(el));
      }

      return res;
    }

    std::string ToJson() const
    {
      auto j = nlohmann::json{ { "isFemale", isFemale },
                               { "raceId", raceId },
                               { "weight", weight },
                               { "skinColor", skinColor },
                               { "hairColor", hairColor },
                               { "headpartIds", headpartIds },
                               { "headTextureSetId", headTextureSetId },
                               { "options", faceMorphs },
                               { "presets", facePresets } };
      j["tints"] = nlohmann::json::array();
      for (auto& tint : tints) {
        j["tints"].push_back(
          nlohmann::json{ { "texturePath", tint.texturePath },
                          { "argb", tint.argb },
                          { "type", tint.type } });
      }
      return j.dump();
    }
  };

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

  const auto& GetPos() const { return pos; }
  const auto& GetAngle() const { return rot; }
  const auto& GetCellOrWorld() const { return cellOrWorld; }
  const auto& IsRaceMenuOpen() const { return isRaceMenuOpen; }
  auto GetLook() const { return look.get(); }

  // Guaranted to have "data" property with look structure
  const std::string& GetLookAsJson()
  {
    if (look && jLookCache.empty())
      jLookCache = look->ToJson();
    return jLookCache;
  }

  void SetPos(const NiPoint3& newPos);
  void SetAngle(const NiPoint3& newAngle);
  void SetRaceMenuOpen(bool isOpen);
  void SetLook(const Look* newLook);

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

  bool isRaceMenuOpen = false;
  std::unique_ptr<Look> look;

  std::string jLookCache;
};

class WorldState
{
  friend class MpActor;

public:
  void AddForm(std::unique_ptr<MpForm> form, uint32_t formId)
  {
    auto& f = forms[formId];
    if (f) {
      throw std::runtime_error(
        static_cast<const std::stringstream&>(std::stringstream()
                                              << "Form with id " << std::hex
                                              << formId << " already exists")
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
      throw std::runtime_error(
        static_cast<const std::stringstream&>(std::stringstream()
                                              << "Form with id " << std::hex
                                              << formId << " doesn't exist")
          .str());
    }

    auto& form = it->second;
    if (!dynamic_cast<FormType*>(form.get())) {
      std::stringstream s;
      s << "Expected form " << std::hex << formId << " to be "
        << MpForm::GetFormType<FormType>() << ", but got "
        << MpForm::GetFormType(form.get());
      throw std::runtime_error(s.str());
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

  template <class F>
  F& GetFormAt(uint32_t formId)
  {
    auto form = LookupFormById(formId);
    if (!form) {
      std::stringstream ss;
      ss << "Form with id " << std::hex << formId << " doesn't exist";
      throw std::runtime_error(ss.str());
    }

    auto typedForm = std::dynamic_pointer_cast<F>(form);
    if (!typedForm) {
      const char* formType = typeid(F).name() + strlen("class Mp");

      std::stringstream ss;
      ss << "Form with id " << std::hex << formId << " is not " << formType;
      throw std::runtime_error(ss.str());
    }

    return *typedForm;
  }

private:
  spp::sparse_hash_map<uint32_t, std::shared_ptr<MpForm>> forms;
  spp::sparse_hash_map<uint32_t, GridImpl<MpActor*>> grids;
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

inline void MpActor::SetRaceMenuOpen(bool isOpen)
{
  isRaceMenuOpen = isOpen;
}

inline void MpActor::SetLook(const Look* newLook)
{
  jLookCache.clear();
  if (newLook) {
    look.reset(new Look(*newLook));
  } else {
    look.reset();
  }
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