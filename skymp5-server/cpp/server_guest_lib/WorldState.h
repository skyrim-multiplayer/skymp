#pragma once
#include "Entity.h"
#include "FormIndex.h"
#include "GridElement.h"
#include "GridInfo.h"
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
private:
  friend class MpObjectReference;
  friend class MpActor;

public:
  using FormCallbacksFactory = std::function<FormCallbacks()>;

public:
  WorldState();
  WorldState(const WorldState&) = delete;
  WorldState& operator=(const WorldState&) = delete;

  Entity& CreateEntity(uint32_t formId);
  Entity GetEntityByFormId(uint32_t formId) const noexcept;
  uint16_t Destroy(uint32_t formId);

  void Clear();
  void AttachEspm(espm::Loader* espm,
                  const FormCallbacksFactory& formCallbacksFactory);
  void AttachSaveStorage(std::shared_ptr<ISaveStorage> saveStorage);
  void AttachScriptStorage(std::shared_ptr<IScriptStorage> scriptStorage);
  bool Valid(Entity entity) const noexcept;
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
  bool Exists(uint32_t formId) const noexcept;
  const std::vector<std::shared_ptr<PartOneListener>>& GetListeners()
    const noexcept;

public:
  std::vector<std::string> espmFiles;
  std::unordered_map<int32_t, std::set<uint32_t>> actorIdByProfileId;
  std::shared_ptr<spdlog::logger> logger;
  std::map<uint32_t, uint32_t> hosters;

private:
  bool AttachEspmRecord(const espm::CombineBrowser& br,
                        espm::RecordHeader* record,
                        const espm::IdMapping& mapping);
  bool LoadForm(uint32_t formId);
  void TickReloot(const std::chrono::system_clock::time_point& now);
  void TickSaveStorage(const std::chrono::system_clock::time_point& now);
  void TickTimers(const std::chrono::system_clock::time_point& now);

  // Only for tests
  auto& GetGrids() const noexcept;

private:
  spp::sparse_hash_map<uint32_t, std::shared_ptr<MpForm>> forms;
  spp::sparse_hash_map<uint32_t, GridInfo> grids;

  // spp::sparse_hash_m-ap<uint32_t, entity_t> entityByFormId;
  spp::sparse_hash_map<uint32_t, Entity> entityByFormId;

  std::unique_ptr<MakeID> formIdxManager;
  std::vector<MpForm*> formByIdxUnreliable;
  std::map<
    std::chrono::system_clock::duration,
    std::list<std::pair<uint32_t, std::chrono::system_clock::time_point>>>
    relootTimers;
  espm::Loader* espm = nullptr;
  FormCallbacksFactory formCallbacksFactory;
  std::unique_ptr<espm::CompressedFieldsCache> espmCache;
  std::vector<std::optional<std::chrono::system_clock::time_point>>
    lastMovUpdateByIdx;
  bool isPapyrusHotReloadEnabled = false;
  std::vector<std::shared_ptr<PartOneListener>> listeners;

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
