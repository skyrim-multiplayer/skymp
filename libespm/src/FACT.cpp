#include "libespm/FACT.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

FACT::Data FACT::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "FULL", 4)) {
        result.fullNameTableID = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "XNAM", 4)) {
        const auto relation =
          *reinterpret_cast<const InterfactionRelation*>(data);
        result.interfactionRelations.push_back(relation);
      } else if (!std::memcmp(type, "DATA", 4)) {
        result.flags = *reinterpret_cast<const Flags*>(data);
      } else if (!std::memcmp(type, "JAIL", 4)) {
        result.prisonMarker = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "WAIT", 4)) {
        result.followerWaitMarker = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "STOL", 4)) {
        result.evidenceChest = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "PLCN", 4)) {
        result.playerBelongingsChest =
          *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "CRGR", 4)) {
        result.crimeGroup = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "JOUT", 4)) {
        result.jailOutfit = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "CRVA", 4)) {
        result.crimeGold = *reinterpret_cast<const CrimeGold*>(data);
      } else if (!std::memcmp(type, "RNAM", 4)) {
        result.ranks.push_back(espm::FACT::Rank());
        result.ranks.back().rankId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "MNAM", 4)) {
        result.ranks.back().maleTitle =
          *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "FNAM", 4)) {
        result.ranks.back().femaleTitle =
          *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "CTDA", 4)) {
        result.conditions.push_back(*reinterpret_cast<const CTDA*>(data));
      }
    },
    compressedFieldsCache);
  return result;
}

}
