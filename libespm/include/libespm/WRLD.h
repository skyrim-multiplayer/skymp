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
    UseLandData = 0x01,     // Use Land Data (DNAM)
    UseLODData = 0x02,      // Use LOD Data (NAM3, NAM4)
    UseMapData = 0x04,      // Use Map Data (MNAM, MODL)
    UseWaterData = 0x08,    // Use Water Data (NAM2)
    unknown = 0x10,
    UseClimateData = 0x20,  // Use Climate Data (CNAM)
    UseSkyCell = 0x40,      // Use Sky Cell
  };

  struct MapData
  {
    struct MapSize
    {
        uint32_t width = 0;  // The usable width of the map
        uint32_t height = 0; // The usable height of the map
        int16_t nwCellX = 0; // (my guess is) max North-West x coord
        int16_t nwCellY = 0; // (my guess is) max North-West y coord
        int16_t seCellX = 0; // (my guess is) max South-EAST x coord
        int16_t seCellY = 0; // (my guess is) max South-EAST y coord
    };

    MapSize mapSize = {};
    float minCameraHeight = 50'000;     // Camera Data (default 50000), new as of Skyrim 1.8, purpose is not yet known. (in my tests, this param controls min and max camera zoom)
    float maxCameraHeight = 80'000;
    float initialCameraPitch = 50;      // Camera Data (default 50) (camera zoom pitch?)
  };

  struct Data
  {
    lstring localNameIndex = 0;                         // The name of this worldspace used in the game
    int16_t centerCellXY[2] = {};                       // X, Y
    formId interiorLightingId = 0;                      // Interior Lighting (LGTM)
    formId encounterZoneId = 0;                         // Encounter Zone (ECZN)
    formId exitLocationId = 0;                          // Location (LCTN)
    formId climateId = 0;                               // Climate reference (CLMT)
    formId waterId = 0;                                 // Water (WATR)
    formId waterLODTypeId = 0;                          // LOD water-type, always a WATR form ID
    float waterLODHeight = 0;                           // LOD oceanwater-level (-14000.0 for Tamriel)
    float LandData[2] = {};                             // Default land- and oceanwater-levels (-27000 & -14000.0 for Tamriel)
    zstring cloudModelFileName;                         // Cloud Model - World model filename (part of MODL struct)
    MapData mapData = {};                               // 16 or 28 byte structure
    float distantLODMult = 0;                           // Distant LOD Multiplier
    Flags flags = Flags::None;
    int32_t bottomLeftCoord[2] = {};                    // Coordinates for the bottom left (South-West) corner of the worldspace (assumed from Oblivion)
    int32_t topRightCoord[2] = {};                      // Coordinates for the top right (North-East) corner of the worldspace (assumed from Oblivion)
    formId parentWorldspaceId = 0;                      // Form ID of the parent worldspace
    UseParentFlags useFlags = UseParentFlags::None;     // Use flags - Set if parts are inherited from parent worldspace WNAM
    float mapMarkerData[4] = {};                        // This field specifies where map markers will appear in relation to the parent (World Map Scale - -1=Hide Map Markers) (Cell X Offset * 4096) (Cell Y Offset * 4096) (Cell Z Offset * 4096)
    zstring diffuseLODName;                             // The name of a texture file
    zstring normalLODName;                              // The name of the normals file matching the TNAM
    formId musicId = 0;                                 // always a (MUSC) form ID
    zstring waterEnvMapName;                            // Water Environment Map
  };

  Data GetData(CompressedFieldsCache& cache) const noexcept;
};

}

#pragma pack(pop)
