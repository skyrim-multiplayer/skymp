#include "libespm/REFR.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

REFR::Data REFR::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "NAME", 4)) {
        result.baseId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "XSCL", 4)) {
        result.scale = *reinterpret_cast<const float*>(data);
      } else if (!std::memcmp(type, "DATA", 4)) {
        result.loc = reinterpret_cast<const LocationalData*>(data);
      } else if (!std::memcmp(type, "XTEL", 4)) {
        result.teleport = reinterpret_cast<const DoorTeleport*>(data);
      } else if (!std::memcmp(type, "XPRM", 4)) {
        result.boundsDiv2 = reinterpret_cast<const float*>(data);
      } else if (!std::memcmp(type, "XCNT", 4)) {
        result.count = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "XAPD", 4)) {
        result.isParentActivationOnly =
          *reinterpret_cast<const uint8_t*>(data);
      } else if (!std::memcmp(type, "XAPR", 4)) {
        ActivationParentInfo info;
        info = *reinterpret_cast<const ActivationParentInfo*>(data);
        result.activationParents.push_back(info);
      } else if (!std::memcmp(type, "XLKR", 4)) {
        [[likely]] if (dataSize == 8) {
          result.linkedRefKeywordId = *reinterpret_cast<const uint32_t*>(data);
          result.linkedRefId = *reinterpret_cast<const uint32_t*>(data + 4);
        } else if (dataSize == 4) {
          result.linkedRefId = *reinterpret_cast<const uint32_t*>(data);
        }
      } else if (!std::memcmp(type, "XOWN", 4)) {
        result.ownerFaction = *reinterpret_cast<const uint32_t*>(data);
      }
    },
    compressedFieldsCache);
  return result;
}

}
