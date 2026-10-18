#pragma once
#include "RecordHeader.h"
#include <vector>

#pragma pack(push, 1)

namespace espm {

class LCTN final : public RecordHeader
{
public:
  static constexpr auto kType = "LCTN";

  // Locations tie together actors, references, keywords, and map metadata for
  // a cell, dungeon, or world area. Physical world data lives in WRLD/CELL.
  //
  // Dawnguard renamed several L* fields to their A* counterparts, but the
  // contents appear to be unchanged.

  struct CellGrid
  {
    // Cell coordinates in the containing worldspace?
    int16_t y = 0;
    int16_t x = 0;
  };
  static_assert(sizeof(CellGrid) == 4);

  struct PopulationRef
  {
    // Actor reference counted as part of this location's population.
    formId actorRefId = 0;

    // Cell or worldspace where the actor reference belongs.
    formId cellOrWorldId = 0;

    // Cell coordinates within the worldspace?
    CellGrid cellGrid = {};
  };
  static_assert(sizeof(PopulationRef) == 12);

  struct UniqueRef
  {
    // Base NPC for a unique actor assigned to this location.
    formId actorBaseId = 0;

    // Placed actor reference for the unique NPC.
    formId actorRefId = 0;

    // Owning location; often points back to this LCTN.
    formId locationId = 0;
  };
  static_assert(sizeof(UniqueRef) == 12);

  struct StaticRef
  {
    // Location reference type (LCRT), used to classify the reference.
    formId locationRefTypeId = 0;

    // Placed REFR or ACHR for this location reference type.
    formId refId = 0;

    // Cell or worldspace where the reference belongs.
    formId cellOrWorldId = 0;

    // Cell coordinates within the worldspace?
    CellGrid cellGrid = {};
  };
  static_assert(sizeof(StaticRef) == 16);

  struct EncounterRef
  {
    // Worldspace used by this encounter area?
    formId worldId = 0;

    // Cell coordinates covered by the encounter area?
    std::vector<CellGrid> cellGrids;
  };

  struct EnablePoint
  {
    // Actor reference tied to this enable point.
    formId actorRefId = 0;

    // Reference enabled or controlled by this entry.
    formId refId = 0;

    // Additional enable data; varies, with the first byte usually zero.
    uint8_t additionalData[4] = {};
  };
  static_assert(sizeof(EnablePoint) == 12);

  struct Color
  {
    // Location color used by game/editor metadata.
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint8_t alpha = 0;
  };
  static_assert(sizeof(Color) == 4);

  struct Data
  {
    // Full in-game name of this location.
    lstring fullNameTableID = 0;

    // Number of location keywords in KWDA.
    uint32_t keywordsCount = 0;

    // Keywords classifying this location, such as location type keywords.
    std::vector<formId> keywordIds;

    // Parent LCTN in the location hierarchy.
    formId parentLocationId = 0;

    // Music type used for this location.
    formId musicId = 0;

    // Faction used for unreported crime in this location.
    formId unreportedCrimeFactionId = 0;

    // World map marker reference for this location.
    formId worldLocationMarkerRefId = 0;

    // Radius around the world map marker.
    float worldLocationRadius = 0;

    // Horse marker reference for this location.
    formId horseMarkerRefId = 0;

    // Color metadata for the location.
    Color color = {};

    // ACPR/LCPR: actor references making up the location population.
    std::vector<PopulationRef> populationRefs;

    // RCPR: actor references; very rare in vanilla data.
    std::vector<formId> actorRefs;

    // ACUN/LCUN: unique actor references assigned to the location.
    std::vector<UniqueRef> uniqueRefs;

    // ACSR/LCSR: typed static refs or actor refs assigned to the location.
    std::vector<StaticRef> staticRefs;

    // ACEC/LCEC: encounter area data for this location?
    std::vector<EncounterRef> encounterRefs;

    // ACEP/LCEP: enable points associated with this location?
    std::vector<EnablePoint> enablePoints;

    // ACID/LCID: unknown form IDs.
    std::vector<formId> unknownRefs;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(LCTN) == sizeof(RecordHeader));

}

#pragma pack(pop)
