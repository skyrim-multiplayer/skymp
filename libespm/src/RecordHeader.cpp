#include "libespm/RecordHeader.h"
#include "libespm/EspmUtils.h"
#include "libespm/RecordHeaderAccess.h"
#include "libespm/ScriptData.h"
#include <cstring>

// debug
#include <iostream>

namespace espm {

uint32_t RecordHeader::GetId() const noexcept
{
  return id;
}

const char* RecordHeader::GetEditorId(
  espm::CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  const char* result = "";
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "EDID", 4))
        result = data;
    },
    compressedFieldsCache);
  return result;
}

void RecordHeader::GetScriptData(
  ScriptData* out, CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  ScriptData res;

  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "VMAD", 4)) {
        res.version = *reinterpret_cast<const uint16_t*>(data);
        res.objFormat = *reinterpret_cast<const uint16_t*>(
          (reinterpret_cast<const uint8_t*>(data) + 2));
        const uint16_t scriptCount = *reinterpret_cast<const uint16_t*>(
          (reinterpret_cast<const uint8_t*>(data) + 4));

        auto p = reinterpret_cast<const uint8_t*>(data) + 6;
        res.scripts.resize(scriptCount);
        utils::FillScriptArray(p, res.scripts, res.objFormat);
      }
    },
    compressedFieldsCache);

  *out = res;
}

std::vector<uint32_t> RecordHeader::GetKeywordIds(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  std::vector<uint32_t> res;
  uint32_t count = 0;

  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "KSIZ", 4)) {
        count = *reinterpret_cast<const uint32_t*>(data);
      }
      if (!std::memcmp(type, "KWDA", 4)) {
        for (uint32_t i = 0; i < count; ++i)
          res.push_back(reinterpret_cast<const uint32_t*>(data)[i]);
      }
    },
    compressedFieldsCache);

  return res;
}

const Type RecordHeader::GetType() const noexcept
{
  return Type{ reinterpret_cast<const char*>(this) - 8 };
}

uint32_t RecordHeader::GetFlags() const noexcept
{
  return flags;
}

uint32_t RecordHeader::GetFieldsSizeSum() const noexcept
{
  const auto ptr = (reinterpret_cast<const int8_t*>(this)) - 4;
  return *reinterpret_cast<const uint32_t*>(ptr);
}

}
