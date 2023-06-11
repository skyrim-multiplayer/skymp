#pragma once
#include "FormIndex.h"
#include "Grid.h"
#include "GridElement.h"
#include "MpChangeForms.h"
#include "MpObjectReference.h"
#include "NiPoint3.h"
#include "PartOneListener.h"
#include "VirtualMachine.h"
#include <Loader.h>
#include <MakeID.h>
#include <MpForm.h>
#include <algorithm>
#include <chrono>
#include <entt/entt.hpp>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <sparsepp/spp.h>
#include <spdlog/spdlog.h>
#include <sstream>

#ifdef AddForm
#  undef AddForm
#endif

class MpActor;
class FormCallbacks;
class MpChangeForm;
class ISaveStorage;
class IScriptStorage;

class WorldState : private entt::registry
{
  friend class MpObjectReference;
  friend class MpActor;

public:
  using entity_t = entt::entity;
  using null_t = entt::null_t;

public:
  using FormCallbacksFactory = std::function<FormCallbacks()>;
  WorldState();
  WorldState(const WorldState&) = delete;
  WorldState& operator=(const WorldState&) = delete;

  void Clear();

  void AttachEspm(espm::Loader* espm,
                  const FormCallbacksFactory& formCallbacksFactory);
  void AttachSaveStorage(std::shared_ptr<ISaveStorage> saveStorage);
  void AttachScriptStorage(std::shared_ptr<IScriptStorage> scriptStorage);


  bool Valid(entity_t entity) const;
  entity_t GetEntityByFormId(uint32_t formId) const noexcept;

  template <typename T, typename... Args>
  decltype(auto) Emplace(uint32_t formId, Args&&... args)
  {
    entity_t entity = GetEntityByFormId(formId);
    if (valid(entity)) {
      throw std::runtime_error(
        fmt::format("Unable to create {} with id {:x}. An entity with this id "
                    "already exists.",
                    typeid(T).name(), formId));
    }
    entity = create();
    entityIdByFormId.insert({ formId, entity });
    return emplace<T>(entity, std::forward<Args>(args)...);
  }

  template <typename... T>
  decltype(auto) Get(uint32_t formId)
  {
    entity_t entity = GetEntityByFormId(formId);
    if (!valid(entity)) {
      throw std::runtime_error(fmt::format(
        "Couldn't find an entity associated with formId {:x}", formId));
    }
    return _get<T...>(entity);
  }

  template <typename... T>
  decltype(auto) Get(entity_t entity)
  {
    return _get<T...>(entity);
  }

  uint16_t Destroy(uint32_t formId)
  {
    entity_t entity = GetEntityByFormId(formId);
    if (!valid(entity)) {
      throw std::runtime_error(fmt::format(
        "Couldn't destroy an entity associated with formId {:x}", formId)s;
    }
    auto& objectReference = get<MpObjectReference>(entity);
    objectReference.BeforeDestroy();
    uint16_t version = destroy(entity, entt::to_version(entity));
    entityByFormId.erase(formId);
    return version;
  }


  void LoadChangeForm(const MpChangeForm& changeForm,
                      const FormCallbacks& callbacks);
  void Tick();
  void RequestReloot(MpObjectReference& ref,
                     std::chrono::system_clock::duration time);
  void RequestSave(MpObjectReference& ref);
  void RegisterForSingleUpdate(const VarValue& self, float seconds);
  Viet::Promise<Viet::Void> SetTimer(float seconds);
  const std::shared_ptr<MpForm>& LookupFormById(uint32_t formId);
  MpForm* LookupFormByIdx(int idx);
  void SendPapyrusEvent(MpForm* form, const char* eventName,
                        const VarValue* arguments, size_t argumentsCount);

  const std::set<MpObjectReference*>& GetReferencesAtPosition(
    uint32_t cellOrWorld, int16_t cellX, int16_t cellY);
  espm::Loader& GetEspm() const;
  bool HasEspm() const;
  espm::CompressedFieldsCache& GetEspmCache();
  IScriptStorage* GetScriptStorage() const;
  VirtualMachine& GetPapyrusVm();
  const std::set<uint32_t>& GetActorsByProfileId(int32_t profileId) const;
  uint32_t GenerateFormId();
  void SetRelootTime(std::string recordType,
                     std::chrono::system_clock::duration dur);
  std::optional<std::chrono::system_clock::duration> GetRelootTime(
    std::string recordType) const;

public:
  std::vector<std::string> espmFiles;
  std::unordered_map<int32_t, std::set<uint32_t>> actorIdByProfileId;
  std::shared_ptr<spdlog::logger> logger;
  std::vector<std::shared_ptr<PartOneListener>> listeners;

  // Only for tests
  auto& GetGrids() { return grids; }
  std::map<uint32_t, uint32_t> hosters;
  std::vector<std::optional<std::chrono::system_clock::time_point>>
    lastMovUpdateByIdx;
  bool isPapyrusHotReloadEnabled = false;

private:
  template <typename... T>
  decltype(auto) _get(entity_t entity)
  {
    auto components = try_get<T...>(entity);
    auto valid = [formId](auto&& component) {
      if (!static_cast<bool>(component)) {
        throw std::runtime_error(
          fmt::format("Couldn't obtain a component of an entity associeated "
                      "with formId {:x}",
                      formId));
      }
    };

    if constexpr (sizeof...(T) == 1) {
      std::invoke(valid, components);
    } else {
      std::apply([valid](auto&&... component) { (..., valid(component)); },
                 components);
    }
    return get<T...>(it->second);
  }

private:
  struct GridInfo
  {
    std::shared_ptr<GridImpl<MpObjectReference*>> grid =
      std::make_shared<GridImpl<MpObjectReference*>>();
    std::map<int16_t, std::map<int16_t, bool>> loadedChunks;
  };
  spp::sparse_hash_map<uint32_t, std::shared_ptr<MpForm>> forms;
  spp::sparse_hash_map<uint32_t, GridInfo> grids;
  spp::sparse_hash_map<uint32_t, entt::entity> entityByFormId;
  std::unique_ptr<MakeID> formIdxManager;
  std::vector<MpForm*> formByIdxUnreliable;
  std::map<
    std::chrono::system_clock::duration,
    std::list<std::pair<uint32_t, std::chrono::system_clock::time_point>>>
    relootTimers;
  espm::Loader* espm = nullptr;
  FormCallbacksFactory formCallbacksFactory;
  std::unique_ptr<espm::CompressedFieldsCache> espmCache;

  bool AttachEspmRecord(const espm::CombineBrowser& br,
                        espm::RecordHeader* record,
                        const espm::IdMapping& mapping);

  bool LoadForm(uint32_t formId);

  void TickReloot(const std::chrono::system_clock::time_point& now);
  void TickSaveStorage(const std::chrono::system_clock::time_point& now);
  void TickTimers(const std::chrono::system_clock::time_point& now);

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
