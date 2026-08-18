#pragma once
#include "RecordHeader.h"
#include <vector>

#pragma pack(push, 1)

namespace espm {

class LCTN final : public RecordHeader
{
public:
  static constexpr auto kType = "LCTN";

  struct CellGrid
  {
    int16_t y = 0;
    int16_t x = 0;
  };
  static_assert(sizeof(CellGrid) == 4);

  struct PopulationRef
  {
    formId actorRefId = 0;
    formId cellOrWorldId = 0;
    CellGrid cellGrid = {};
  };
  static_assert(sizeof(PopulationRef) == 12);

  struct UniqueRef
  {
    formId actorBaseId = 0;
    formId actorRefId = 0;
    formId locationId = 0;
  };
  static_assert(sizeof(UniqueRef) == 12);

  struct StaticRef
  {
    formId locationRefTypeId = 0;
    formId markerRefId = 0;
    formId cellOrWorldId = 0;
    CellGrid cellGrid = {};
  };
  static_assert(sizeof(StaticRef) == 16);

  struct EncounterRef
  {
    formId cellOrWorldId = 0;
    std::vector<CellGrid> coordinates;
  };

  struct EnablePoint
  {
    formId actorRefId = 0;
    formId refId = 0;
    CellGrid cellGrid = {};
  };
  static_assert(sizeof(EnablePoint) == 12);

  struct Color
  {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint8_t alpha = 0;
  };
  static_assert(sizeof(Color) == 4);

  struct Data
  {
    const char* editorId = "";
    lstring fullNameTableID = 0;
    uint32_t keywordsCount = 0;
    std::vector<formId> keywordIds;
    formId parentLocationId = 0;
    formId musicId = 0;
    formId unreportedCrimeFactionId = 0;
    formId worldLocationMarkerRefId = 0;
    float worldLocationRadius = 0;
    formId horseMarkerRefId = 0;
    Color color = {};

    std::vector<PopulationRef> populationRefs;
    std::vector<formId> referencePersistentRefs;
    std::vector<UniqueRef> uniqueRefs;
    std::vector<StaticRef> staticRefs;
    std::vector<EncounterRef> encounterRefs;
    std::vector<EnablePoint> enablePoints;
    std::vector<formId> markerRefs;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(LCTN) == sizeof(RecordHeader));

}

#pragma pack(pop)
