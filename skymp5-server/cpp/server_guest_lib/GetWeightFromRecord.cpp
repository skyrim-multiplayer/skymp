#include "GetWeightFromRecord.h"

#include "libespm/ARMO.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/Convert.h"
#include "libespm/INGR.h"
#include "libespm/LIGH.h"
#include "libespm/RecordHeader.h"
#include "libespm/WEAP.h"

float GetWeightFromRecord(const espm::RecordHeader* record,
                          espm::CompressedFieldsCache& cache)
{
  if (auto* weap = espm::Convert<espm::WEAP>(record)) {
    auto data = weap->GetData(cache);
    return data.weapData->weight;
  }

  if (auto* ligh = espm::Convert<espm::LIGH>(record)) {
    auto data = ligh->GetData(cache);
    return data.data.weight;
  }

  if (auto* armo = espm::Convert<espm::ARMO>(record)) {
    auto data = armo->GetData(cache);
    return data.weight;
  }

  if (auto* ingr = espm::Convert<espm::INGR>(record)) {
    auto data = ingr->GetData(cache);
    return data.itemData.weight;
  }
  return 0.f;
}
