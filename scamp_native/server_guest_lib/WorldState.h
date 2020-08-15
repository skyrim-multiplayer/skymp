#pragma once
#include "FormIndex.h"
#include "Grid.h"
#include "GridElement.h"
#include "NiPoint3.h"
#include <Loader.h>
#include <MakeID.h>
#include <MpForm.h>
#include <algorithm>
#include <sparsepp/spp.h>
#include <sstream>

#ifdef AddForm
#  undef AddForm
#endif

class MpObjectReference;
class MpActor;

class WorldState
{
  friend class MpObjectReference;
  friend class MpActor;

public:
  WorldState() = default;
  WorldState(const WorldState&) = delete;
  WorldState& operator=(const WorldState&) = delete;

  void Clear();

  void AttachEspm(espm::Loader* espm);

  void AddForm(std::unique_ptr<MpForm> form, uint32_t formId,
               bool skipChecks = false);

  const std::shared_ptr<MpForm>& LookupFormById(uint32_t formId);

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

private:
  spp::sparse_hash_map<uint32_t, std::shared_ptr<MpForm>> forms;
  spp::sparse_hash_map<uint32_t, GridImpl<MpObjectReference*>> grids;
  std::unique_ptr<MakeID> formIdxManager;
  espm::Loader* espm = nullptr;
};
