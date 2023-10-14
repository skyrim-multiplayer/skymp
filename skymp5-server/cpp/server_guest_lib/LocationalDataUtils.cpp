#include "LocationalDataUtils.h"
#include "NiPoint3.h"

#include "libespm/GroupUtils.h"
#include "libespm/Utils.h"

const NiPoint3& LocationalDataUtils::GetPos(
  const espm::REFR::LocationalData* locationalData)
{
  return *reinterpret_cast<const NiPoint3*>(locationalData->pos);
}

NiPoint3 LocationalDataUtils::GetRot(
  const espm::REFR::LocationalData* locationalData)
{
  static const auto kPi = std::acos(-1.f);
  return { locationalData->rotRadians[0] / kPi * 180.f,
           locationalData->rotRadians[1] / kPi * 180.f,
           locationalData->rotRadians[2] / kPi * 180.f };
}

uint32_t LocationalDataUtils::GetWorldOrCell(
  const espm::CombineBrowser& br, const espm::LookupResult& refrLookupRes)
{
  auto mapping = br.GetCombMapping(refrLookupRes.fileIdx);
  uint32_t worldOrCell = espm::utils::GetMappedId(
    espm::GetWorldOrCell(br, refrLookupRes.rec), *mapping);
  return worldOrCell;
}
