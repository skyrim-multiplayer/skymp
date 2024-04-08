#pragma once
#include "CompressedFieldsCache.h"
#include "DataTypes.h"
#include "Type.h"

#pragma pack(push, 1)

namespace espm {

struct ScriptData;

class RecordHeader
{
  friend class Browser;
  friend class RecordHeaderAccess;

public:
  const Type GetType() const noexcept;
  // Please use for tests only
  // Do not rely on Skyrim record flags format
  uint32_t GetFlags() const noexcept;
  uint32_t GetId() const noexcept;
  uint16_t GetVersion() const noexcept;

  const char* GetEditorId(
    CompressedFieldsCache& compressedFieldsCache) const noexcept;
  void GetScriptData(
    ScriptData* out,
    CompressedFieldsCache& compressedFieldsCache) const noexcept;
  std::vector<uint32_t> GetKeywordIds(
    CompressedFieldsCache& compressedFieldsCache) const noexcept;

private:
  RecordHeader() = delete;
  RecordHeader(const RecordHeader&) = delete;
  void operator=(const RecordHeader&) = delete;

  uint32_t GetFieldsSizeSum() const noexcept;

protected:
  uint32_t flags;
  uint32_t id;
  uint32_t revision;
  uint16_t version;
  uint16_t unk;
};

static_assert(sizeof(RecordHeader) == 16);

}

#pragma pack(pop)
