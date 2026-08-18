#include "libespm/LCTN.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {
namespace {

template <class T>
void AppendArray(std::vector<T>& out, uint32_t dataSize,
                 const char* data) noexcept
{
  const auto count = dataSize / sizeof(T);
  const auto typedData = reinterpret_cast<const T*>(data);
  out.insert(out.end(), typedData, typedData + count);
}

void AppendEncounterRef(std::vector<LCTN::EncounterRef>& out,
                        uint32_t dataSize, const char* data) noexcept
{
  if (dataSize < sizeof(formId)) {
    return;
  }

  LCTN::EncounterRef encounterRef;
  encounterRef.worldId = *reinterpret_cast<const formId*>(data);

  const auto pairsSize = dataSize - sizeof(formId);
  const auto pairsCount = pairsSize / sizeof(LCTN::CellGrid);
  const auto pairs =
    reinterpret_cast<const LCTN::CellGrid*>(data + sizeof(formId));
  encounterRef.cellGrids.insert(encounterRef.cellGrids.end(), pairs,
                                pairs + pairsCount);

  out.push_back(encounterRef);
}

}

LCTN::Data LCTN::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "ACPR", 4) || !std::memcmp(type, "LCPR", 4)) {
        AppendArray(result.populationRefs, dataSize, data);
      } else if (!std::memcmp(type, "RCPR", 4)) {
        AppendArray(result.actorRefs, dataSize, data);
      } else if (!std::memcmp(type, "ACUN", 4) ||
                 !std::memcmp(type, "LCUN", 4)) {
        AppendArray(result.uniqueRefs, dataSize, data);
      } else if (!std::memcmp(type, "ACSR", 4) ||
                 !std::memcmp(type, "LCSR", 4)) {
        AppendArray(result.staticRefs, dataSize, data);
      } else if (!std::memcmp(type, "ACEC", 4) ||
                 !std::memcmp(type, "LCEC", 4)) {
        AppendEncounterRef(result.encounterRefs, dataSize, data);
      } else if (!std::memcmp(type, "ACEP", 4) ||
                 !std::memcmp(type, "LCEP", 4)) {
        AppendArray(result.enablePoints, dataSize, data);
      } else if (!std::memcmp(type, "ACID", 4) ||
                 !std::memcmp(type, "LCID", 4)) {
        AppendArray(result.unknownRefs, dataSize, data);
      } else if (!std::memcmp(type, "FULL", 4)) {
        result.fullNameTableID = *reinterpret_cast<const lstring*>(data);
      } else if (!std::memcmp(type, "KSIZ", 4)) {
        result.keywordsCount = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "KWDA", 4)) {
        AppendArray(result.keywordIds, dataSize, data);
      } else if (!std::memcmp(type, "PNAM", 4)) {
        result.parentLocationId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "NAM1", 4)) {
        result.musicId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "FNAM", 4)) {
        result.unreportedCrimeFactionId =
          *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "MNAM", 4)) {
        result.worldLocationMarkerRefId =
          *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "RNAM", 4)) {
        result.worldLocationRadius = *reinterpret_cast<const float*>(data);
      } else if (!std::memcmp(type, "NAM0", 4)) {
        result.horseMarkerRefId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "CNAM", 4)) {
        result.color = *reinterpret_cast<const Color*>(data);
      }
    },
    compressedFieldsCache);
  return result;
}

}
