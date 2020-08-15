#pragma once
#include <array>
#include <cstdint>
#include <cstring> // memcmp
#include <functional>
#include <optional>
#include <string>
#include <vector>

#pragma pack(push, 1)

namespace espm {

class CompressedFieldsCache
{
public:
  CompressedFieldsCache();
  ~CompressedFieldsCache();

  struct Impl;
  Impl* const pImpl;
};

union CellOrGridPos
{
  uint32_t cellId = 0;
  struct
  {
    int16_t y;
    int16_t x;
  } pos;
};

class GroupHeader;
class RecordHeader;

class Browser
{
public:
  Browser(void* fileContent, size_t length);
  ~Browser();

  RecordHeader* LookupById(uint32_t formId) const noexcept;

  std::pair<espm::RecordHeader**, size_t> FindNavMeshes(
    uint32_t worldSpaceId, CellOrGridPos cellOrGridPos) const noexcept;

private:
  struct Impl;
  Impl* const pImpl;

  bool ReadAny(void* parentGrStack);

  Browser(const Browser&) = delete;
  void operator=(const Browser&) = delete;
};

enum class GroupType : uint32_t
{
  TOP,
  WORLD_CHILDREN,
  INTERIOR_CELL_BLOCK,
  INTERIOR_CELL_SUBBLOCK,
  EXTERIOR_CELL_BLOCK,
  EXTERIOR_CELL_SUBBLOCK,
  CELL_CHILDREN,
  TOPIC_CHILDREN,
  CELL_PERSISTENT_CHILDREN,
  CELL_TEMPORARY_CHILDREN,
  CELL_VISIBLE_DISTANT_CHILDREN
};
static_assert((int)GroupType::CELL_VISIBLE_DISTANT_CHILDREN == 10);

class GroupHeader
{
  friend class Browser;

public:
  bool GetXY(int16_t& outX, int16_t& outY) const noexcept;
  const char* GetRecordsType() const noexcept;
  bool GetBlockNumber(int32_t& outBlockNum) const noexcept;
  bool GetSubBlockNumber(int32_t& outSubBlockNum) const noexcept;
  bool GetParentWRLD(uint32_t& outId) const noexcept;
  bool GetParentCELL(uint32_t& outId) const noexcept;
  bool GetParentDIAL(uint32_t& outId) const noexcept;

  using RecordVisitor = std::function<bool(espm::RecordHeader*)>;
  void ForEachRecord(const RecordVisitor& visitor) const
    noexcept; // Return true from visitor to break loop

  GroupType GetGroupType() const noexcept { return grType; }

private:
  char label[4];
  GroupType grType;

  uint8_t day;
  uint8_t months;
  uint16_t unknown;
  uint16_t version;
  uint16_t unknown2;

  // We write pointer to GroupDataInternal here
  uint64_t& GroupDataPtrStorage() const noexcept { return *(uint64_t*)&day; }

  GroupHeader() = delete;
  GroupHeader(const GroupHeader&) = delete;
  void operator=(const GroupHeader&) = delete;
};
static_assert(sizeof(GroupType) == 4);
static_assert(sizeof(GroupHeader) == 16);

using GroupStack = std::vector<espm::GroupHeader*>;

using IdMapping = std::array<uint8_t, 256>;
uint32_t GetMappedId(uint32_t id, const IdMapping& mapping) noexcept;

class Type
{
public:
  // Type object doesn't own this pointer
  Type(const char* type_)
    : type(type_){};

  bool operator==(const char* rhs) const noexcept
  {
    return !memcmp(type, rhs, 4);
  }

  std::string ToString() const noexcept { return std::string(type, 4); }

private:
  const char* type;
};

class RecordHeader
{
  friend class espm::Browser;

public:
  uint32_t GetId() const noexcept;
  Type GetType() const noexcept;
  const GroupStack& GetParentGroups() const noexcept;

  // Please use for tests only
  // Do not rely on Skyrim record flags format
  uint32_t GetFlags() const noexcept;

private:
  uint32_t flags;
  uint32_t id;
  uint32_t revision;
  uint16_t version;
  uint16_t unk;

  // We write pointer to std::vector<GroupHeader *> here
  uint64_t& GroupStackPtrStorage() const noexcept
  {
    return *(uint64_t*)&revision;
  }

  uint32_t GetFieldsSizeSum() const noexcept;

  RecordHeader() = delete;
  RecordHeader(const RecordHeader&) = delete;
  void operator=(const RecordHeader&) = delete;

protected:
  using FieldVisitor =
    std::function<void(const char* type, uint32_t dataSize, const char* data)>;
  void ForEachField(const FieldVisitor& f,
                    CompressedFieldsCache* optionalCompressedFieldsCache =
                      nullptr) const noexcept;
};
static_assert(sizeof(RecordHeader) == 16);

// Helpers/utilities

inline GroupHeader* GetExteriorWorldGroup(const RecordHeader* rec)
{
  for (auto gr : rec->GetParentGroups()) {
    if (gr->GetGroupType() == GroupType::WORLD_CHILDREN)
      return gr;
  }
  return nullptr;
}

inline GroupHeader* GetCellGroup(const RecordHeader* rec)
{
  for (auto gr : rec->GetParentGroups()) {
    auto grType = gr->GetGroupType();
    if (grType != GroupType::CELL_CHILDREN &&
        grType != GroupType::CELL_PERSISTENT_CHILDREN &&
        grType != GroupType::CELL_TEMPORARY_CHILDREN &&
        grType != GroupType::CELL_VISIBLE_DISTANT_CHILDREN) {
      continue;
    }
    return gr;
  }
  return nullptr;
}
}

namespace espm {
template <class RecordT>
const RecordT* Convert(const RecordHeader* source)
{
  if (source && source->GetType() == RecordT::type) {
    return (const RecordT*)source;
  }
  return nullptr;
}
}

namespace espm {
class TES4 : public RecordHeader
{
public:
  inline static const auto type = "TES4";

  // Header
  struct Header
  {
    float version = 0;
    int32_t numRecords = 0;
    uint32_t nextObjectId = 0;
  };
  static_assert(sizeof(Header) == 12);

  struct Data
  {
    const Header* header = nullptr;
    const char* author = "";
    const char* description = "";
    std::vector<const char*> masters;
  };

  Data GetData() const noexcept;
};
static_assert(sizeof(TES4) == sizeof(RecordHeader));

class REFR : public RecordHeader
{
public:
  inline static const auto type = "REFR";

  struct LocationalData
  {
    float pos[3];
    float rotRadians[3];
  };

  struct Data
  {
    uint32_t baseId = 0;
    float scale = 1;
    const LocationalData* loc = nullptr;
  };

  Data GetData() const noexcept;
};
static_assert(sizeof(REFR) == sizeof(RecordHeader));

class CONT : public RecordHeader
{
public:
  inline static const auto type = "CONT";

  struct ContainerObject
  {
    uint32_t formId = 0;
    uint32_t count = 0;
  };

  struct Data
  {
    const char* editorId = "";
    const char* fullName = "";
    std::vector<ContainerObject> objects;
  };

  Data GetData() const noexcept;
};
static_assert(sizeof(CONT) == sizeof(RecordHeader));

class LVLI : public RecordHeader
{
public:
  inline static const auto type = "LVLI";

  enum LeveledItemFlags
  {
    AllLevels = 0x01, //(sets it to calculate for all entries < player level,
                      // choosing randomly from all the entries under)
    Each = 0x02, // (sets it to repeat a check every time the list is called
                 // (if it's called multiple times), otherwise it will use the
                 // same result for all counts.)
    UseAll = 0x04, // (use all entries when the list is called)
    SpecialLoot = 0x08,
  };

  struct Entry
  {
    char type[4] = { 'L', 'V', 'L', 'O' };
    uint16_t dataSize = 0;
    uint32_t level = 0;
    uint32_t formId = 0;
    uint32_t count = 0;
  };

  struct Data
  {
    const char* editorId = "";
    uint8_t chanceNone = 0;
    uint8_t leveledItemFlags;
    uint32_t chanceNoneGlobalId = 0;
    uint8_t numEntries = 0;
    Entry* entries = nullptr;
  };

  Data GetData() const noexcept;
};
static_assert(sizeof(LVLI) == sizeof(RecordHeader));
static_assert(sizeof(LVLI::Entry) == 18);

class NAVM : public RecordHeader
{
public:
  inline static const auto type = "NVNM";

  class Vertices
  {
  public:
    Vertices(const void* nvnmField);

    const std::array<float, 3>* begin() const noexcept;
    const std::array<float, 3>* end() const noexcept;

  private:
    const int32_t* numVerticesPtr = nullptr;
    const std::array<float, 3>* beginPtr = nullptr;
    const void* nvnmField;
  };

  struct Data
  {
    std::optional<Vertices> vertices;
    uint32_t worldSpaceId = 0;
    CellOrGridPos cellOrGridPos;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(REFR) == sizeof(RecordHeader));
}

#pragma pack(pop)