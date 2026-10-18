#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class REFR : public RecordHeader
{
public:
  static constexpr auto kType = "REFR";

  struct LocationalData
  {
    float pos[3];
    float rotRadians[3];
  };

  struct DoorTeleport
  {
    uint32_t destinationDoor = 0;
    float pos[3];
    float rotRadians[3];
  };

  struct ActivationParentInfo
  {
    uint32_t refrId = 0;
    float delay = 0.f;
  };

  enum class MapMarkerFlags : uint8_t
  {
    None = 0,
    Visible = 1 << 0,
    CanTravelTo = 1 << 1,
    ShowAllHidden = 1 << 2
  };

  enum class MapMarkerType : uint16_t
  {
    None = 0,
    City = 1,
    Town = 2,
    Settlement = 3,
    Cave = 4,
    Camp = 5,
    Fort = 6,
    NordicRuins = 7,
    DwemerRuin = 8,
    Shipwreck = 9,
    Grove = 10,
    Landmark = 11,
    DragonLair = 12,
    Farm = 13,
    WoodMill = 14,
    Mine = 15,
    ImperialCamp = 16,
    StormcloakCamp = 17,
    Doomstone = 18,
    WheatMill = 19,
    Smelter = 20,
    Stable = 21,
    ImperialTower = 22,
    Clearing = 23,
    Pass = 24,
    Altar = 25,
    Rock = 26,
    Lighthouse = 27,
    OrcStronghold = 28,
    GiantCamp = 29,
    Shack = 30,
    NordicTower = 31,
    NordicDwelling = 32,
    Docks = 33,
    Shrine = 34,
    RiftenCastle = 35,
    RiftenCapitol = 36,
    WindhelmCastle = 37,
    WindhelmCapitol = 38,
    WhiterunCastle = 39,
    WhiterunCapitol = 40,
    SolitudeCastle = 41,
    SolitudeCapitol = 42,
    MarkarthCastle = 43,
    MarkarthCapitol = 44,
    WinterholdCastle = 45,
    WinterholdCapitol = 46,
    MorthalCastle = 47,
    MorthalCapitol = 48,
    FalkreathCastle = 49,
    FalkreathCapitol = 50,
    DawnstarCastle = 51,
    DawnstarCapitol = 52,
    Dlc02TempleOfMiraak = 53,
    Dlc02RavenRock = 54,
    Dlc02StandingStones = 55,
    Dlc02TelvanniTower = 56,
    Dlc02ToSkyrim = 57,
    Dlc02ToSolstheim = 58,
    Dlc02CastleKarstaag = 59
  };
  static_assert(sizeof(MapMarkerType) == 2);

  struct Data
  {
    uint32_t baseId = 0;
    float scale = 1;
    const LocationalData* loc = nullptr;
    const DoorTeleport* teleport = nullptr;
    const float* boundsDiv2 = nullptr;
    uint32_t count = 0;
    uint8_t isParentActivationOnly = 0;
    std::vector<ActivationParentInfo> activationParents;
    uint32_t linkedRefKeywordId = 0;
    uint32_t linkedRefId = 0;
    uint32_t ownerFaction = 0;

    // FULL: in-game name of the map marker.
    lstring mapMarkerName = 0;

    // FNAM: map marker flags; visible, can travel to, or show all hidden.
    MapMarkerFlags mapMarkerFlags = MapMarkerFlags::None;

    // TNAM: icon type for the map marker.
    MapMarkerType mapMarkerType = MapMarkerType::None;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(REFR) == sizeof(RecordHeader));

}

#pragma pack(pop)
