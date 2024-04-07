#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class WRLD final : public RecordHeader
{
public:
  static constexpr auto kType = "WRLD";

  enum class RecordFlags : RecordFlagsType
  {
    CantWait = 0x80000
  };

  enum class Flags : uint8_t
  {
    None = 0,
    SmallWorld = 0x01,
    CantFastTravelFromHere = 0x02,
    unknown = 0x04,
    NoLODWater = 0x08,
    NoLandscape = 0x10,
    NoSky = 0x20,
    FixedDimensions = 0x40,
    NoGrass = 0x80
  };

  enum class UseParentFlags : std::uint16_t
  {
    None = 0,
    UseLandData = 0x01,  // Use Land Data (DNAM)
    UseLODData = 0x02,   // Use LOD Data (NAM3, NAM4)
    UseMapData = 0x04,   // Use Map Data (MNAM, MODL)
    UseWaterData = 0x08, // Use Water Data (NAM2)
    unknown = 0x10,
    UseClimateData = 0x20, // Use Climate Data (CNAM)
    UseSkyCell = 0x40,     // Use Sky Cell
  };

  struct MapData
  {
    struct MapSize
    {
      // The usable width of the map
      uint32_t width = 0;

      // The usable height of the map
      uint32_t height = 0;

      // (my guess is) max North-West x coord
      int16_t nwCellX = 0;

      // (my guess is) max North-West y coord
      int16_t nwCellY = 0;

      // (my guess is) max South-EAST x coord
      int16_t seCellX = 0;

      // (my guess is) max South-EAST y coord
      int16_t seCellY = 0;
    };

    MapSize mapSize = {};

    // Camera Data (default 50000), new as of Skyrim 1.8, purpose is not yet
    // known. (in my tests, this param controls min and max camera zoom)
    float minCameraHeight = 50'000;
    float maxCameraHeight = 80'000;

    // Camera Data (default 50) (camera zoom pitch?)
    float initialCameraPitch = 50;
  };

  struct Data
  {
    // The name of this worldspace used in the game
    lstring localNameIndex = 0;

    // X, Y
    int16_t centerCellXY[2] = {};

    // Interior Lighting reference (LGTM)
    formId interiorLightingId = 0;

    // Encounter Zone reference (ECZN)
    formId encounterZoneId = 0;

    // Location reference (LCTN)
    formId exitLocationId = 0;

    // Climate reference (CLMT)
    formId climateId = 0;

    // Water reference (WATR)
    formId waterId = 0;

    // LOD water-type, always a WATR form ID
    formId waterLODTypeId = 0;

    // LOD oceanwater-level (-14000.0 for Tamriel)
    float waterLODHeight = 0;

    // Default land- and oceanwater-levels (-27000 & -14000.0 for Tamriel)
    float LandData[2] = {};

    // Cloud Model - World model filename (part of MODL struct)
    zstring cloudModelFileName;

    // 16 or 28 byte structure
    MapData mapData = {};

    // Distant LOD Multiplier
    float distantLODMult = 0;

    Flags flags = Flags::None;

    // Coordinates for the bottom left (South-West) corner of the worldspace
    // (assumed from Oblivion)
    int32_t bottomLeftCoord[2] = {};

    // Coordinates for the top right (North-East) corner of the worldspace
    // (assumed from Oblivion)
    int32_t topRightCoord[2] = {};

    // Form ID of the parent worldspace
    formId parentWorldspaceId = 0;

    // Use flags - Set if parts are inherited from parent worldspace WNAM
    UseParentFlags useFlags = UseParentFlags::None;

    // This field specifies where map markers will appear in relation to the
    // parent (World Map Scale - -1=Hide Map Markers) (Cell X Offset * 4096)
    // (Cell Y Offset * 4096) (Cell Z Offset * 4096)
    float mapMarkerData[4] = {};

    // The name of a texture file
    zstring diffuseLODName;

    // The name of the normals file matching the TNAM
    zstring normalLODName;

    // Always a (MUSC) form ID
    formId musicId = 0;

    // Water Environment Map
    zstring waterEnvMapName;
  };

  Data GetData(CompressedFieldsCache& cache) const noexcept;
};

}

#pragma pack(pop)
