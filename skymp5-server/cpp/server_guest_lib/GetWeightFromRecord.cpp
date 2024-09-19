#include "GetWeightFromRecord.h"

#include "libespm/ALCH.h"
#include "libespm/AMMO.h"
#include "libespm/ARMO.h"
#include "libespm/BOOK.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/Convert.h"
#include "libespm/INGR.h"
#include "libespm/LIGH.h"
#include "libespm/MISC.h"
#include "libespm/RecordHeader.h"
#include "libespm/SCRL.h"
#include "libespm/SLGM.h"
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

  if (auto* alch = espm::Convert<espm::ALCH>(record)) {
    auto data = alch->GetData(cache);
    return data.weight;
  }

  if (auto* book = espm::Convert<espm::BOOK>(record)) {
    auto data = book->GetData(cache);
    return data.weight;
  }

  // TODO: add unit test
  if (auto* ammo = espm::Convert<espm::AMMO>(record)) {
    auto data = ammo->GetData(cache);
    return data.weight;
  }

  // TODO: add unit test
  if (auto* scroll = espm::Convert<espm::SCRL>(record)) {
    auto data = scroll->GetData(cache);
    return data.data.weight;
  }

  // TODO: add unit test
  if (auto* slgm = espm::Convert<espm::SLGM>(record)) {
    auto data = slgm->GetData(cache);
    return data.weight;
  }

  // TODO: add unit test
  if (auto* misc = espm::Convert<espm::MISC>(record)) {
    auto data = misc->GetData(cache);
    return data.weight;
  }

  return 0.f;
}
