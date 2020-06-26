#pragma once
#include "NiPoint3.h"
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

private:
  uint32_t formId = 0; // Assigned by WorldState::AddForm
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
  MpActor(const LocationalData& locationalData_)
  {
    static_cast<LocationalData&>(*this) = locationalData_;
  }

  const auto& GetPos() { return pos; }
  const auto& GetAngle() { return rot; }
  const auto& GetCellOrWorld() { return cellOrWorld; }
};

class WorldState
{
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
    f = std::move(form);
  }

  template <class FormType = MpForm>
  void DestroyForm(uint32_t formId)
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
};