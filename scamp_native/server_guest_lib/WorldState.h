#pragma once
#include "NiPoint3.h"
#include <sparsepp/spp.h>
#include <sstream>

#ifdef AddForm
#  undef AddForm
#endif

class MpForm
{
public:
  virtual ~MpForm() = default;
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
    f = std::move(form);
  }

  MpForm* LookupFormById(uint32_t formId)
  {
    auto it = forms.find(formId);
    if (it == forms.end())
      return nullptr;
    return it->second.get();
  }

private:
  spp::sparse_hash_map<uint32_t, std::unique_ptr<MpForm>> forms;
};