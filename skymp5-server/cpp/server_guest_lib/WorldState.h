#pragma once
#include "FormIndex.h"
#include "Grid.h"
#include "GridElement.h"
#include "MpChangeForms.h"
#include "MpForm.h"
#include "MpObjectReference.h"
#include "NiPoint3.h"
#include "PartOneListener.h"
#include "libespm/Loader.h"
#include "papyrus-vm/VirtualMachine.h"
#include <MakeID.h-1.0.2>
#include <algorithm>
#include <chrono>
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

class WorldState
{
  friend class MpObjectReference;
  friend class MpActor;

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

  void AddForm(std::unique_ptr<MpForm> form, uint32_t formId,
               bool skipChecks = false,
               const MpChangeForm* optionalChangeFormToApply = nullptr);

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

  template <class F>
  F& GetFormAt(uint32_t formId)
  {
    auto form = LookupFormById(formId);
    if (!form) {
      throw std::runtime_error(
        fmt::format("Form with id {:#x} doesn't exist", formId));
    }

    auto typedForm = std::dynamic_pointer_cast<F>(form);
    if (!typedForm) {
      if constexpr (std::is_same_v<F, MpActor>) {
        if (auto ref = std::dynamic_pointer_cast<MpObjectReference>(form)) {
          auto pos = ref->GetPos();
          spdlog::warn(
            "Specified Form is ObjectReference, but we tried to treat it as "
            "Actor, likely because of a client bug. formId={:#x}, "
            "baseId={:#x}, pos is {:.2f} {:.2f} {:.2f} at {}",
            formId, ref->GetBaseId(), pos.x, pos.y, pos.z,
            ref->GetCellOrWorld().ToString());
        }
      }
      throw std::runtime_error(
        fmt::format("Form with id {:#x} is not {} (actually it is {})", formId,
                    F::Type(), MpForm::GetFormType(&*form)));
    }

    return *typedForm;
  };

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
  };

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
  struct GridInfo
  {
    std::shared_ptr<GridImpl<MpObjectReference*>> grid =
      std::make_shared<GridImpl<MpObjectReference*>>();
    std::map<int16_t, std::map<int16_t, bool>> loadedChunks;
  };

  spp::sparse_hash_map<uint32_t, std::shared_ptr<MpForm>> forms;
  spp::sparse_hash_map<uint32_t, GridInfo> grids;
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
