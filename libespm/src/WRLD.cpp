#include "libespm/WRLD.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

WRLD::Data WRLD::GetData(CompressedFieldsCache& cache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "FULL", 4)) {
        result.localNameIndex = *reinterpret_cast<const lstring*>(data);
      } else if (!std::memcmp(type, "WCTR", 4)) {
        // std::copy_n(data, std::size(result.centerCellXY),
        // result.centerCellXY);
      } else if (!std::memcmp(type, "LTMP", 4)) {
        result.interiorLightingId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "XEZN", 4)) {
        result.encounterZoneId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "XLCN", 4)) {
        result.exitLocationId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "CNAM", 4)) {
        result.climateId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "NAM2", 4)) {
        result.waterId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "NAM3", 4)) {
        result.waterLODTypeId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "NAM4", 4)) {
        result.waterLODHeight = *reinterpret_cast<const float*>(data);
      } else if (!std::memcmp(type, "DNAM", 4)) {
        // std::copy_n(data, std::size(result.LandData), result.LandData);
      } else if (!std::memcmp(type, "MODL", 4)) {
        result.cloudModelFileName = reinterpret_cast<const char*>(data);
      } else if (!std::memcmp(type, "MNAM", 4)) {
        if (size == sizeof(WRLD::MapData::MapSize)) {
          result.mapData.mapSize =
            *reinterpret_cast<const WRLD::MapData::MapSize*>(data);
        } else {
          result.mapData = *reinterpret_cast<const WRLD::MapData*>(data);
        }
      } else if (!std::memcmp(type, "NAMA", 4)) {
        result.distantLODMult = *reinterpret_cast<const float*>(data);
      } else if (!std::memcmp(type, "DATA", 4)) {
        result.flags = *reinterpret_cast<const WRLD::Flags*>(data);
      } else if (!std::memcmp(type, "WNAM", 4)) {
        result.parentWorldspaceId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "PNAM", 4)) {
        result.useFlags = *reinterpret_cast<const WRLD::UseParentFlags*>(data);
      } else if (!std::memcmp(type, "ONAM", 4)) {
        // std::copy_n(data, std::size(result.mapMarkerData),
        //             result.mapMarkerData);
      } else if (!std::memcmp(type, "TNAM", 4)) {
        result.diffuseLODName = reinterpret_cast<const char*>(data);
      } else if (!std::memcmp(type, "UNAM", 4)) {
        result.normalLODName = reinterpret_cast<const char*>(data);
      } else if (!std::memcmp(type, "ZNAM", 4)) {
        result.musicId = *reinterpret_cast<const formId*>(data);
      } else if (!std::memcmp(type, "XWEM", 4)) {
        result.waterEnvMapName = reinterpret_cast<const char*>(data);
      }
    },
    cache);
  return result;
}

}
