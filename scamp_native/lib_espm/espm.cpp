#include "ZlibUtils.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <sparsepp/spp.h>

#include "espm.h"

static_assert(sizeof(char) == 1);

namespace {
uint64_t MakeUInt64(uint32_t high, uint32_t low)
{
  return (((uint64_t)high) << 32) | ((uint64_t)low);
}

class NavMeshKey
{
public:
  NavMeshKey(uint32_t worldSpaceId, espm::CellOrGridPos cellOrGridPos)
    : v(MakeUInt64(worldSpaceId, cellOrGridPos.cellId))
  {
  }

  operator uint64_t() const noexcept { return v; }

private:
  const uint64_t v;
};
}

namespace espm {

struct CompressedFieldsCache::Impl
{
  struct Entry
  {
    std::shared_ptr<std::vector<uint8_t>> decompressedFieldsHolder;
  };

  spp::sparse_hash_map<const RecordHeader*, Entry> data;
};

CompressedFieldsCache::CompressedFieldsCache()
  : pImpl(new Impl)
{
}

CompressedFieldsCache::~CompressedFieldsCache()
{
  delete pImpl;
}

#pragma pack(push, 1)
struct FieldHeader
{
  char type[4];
  uint16_t dataSize;
};
static_assert(sizeof(FieldHeader) == 6);
#pragma pack(pop)
}

enum RecordFlags : uint32_t
{
  Deleted = 0x00000020,
  Constant = 0x00000040,
  REFR_HiddenFromLocalMap = 0x00000040,
  MustUpdateAnims = 0x00000100,
  REFR_Inaccessible = 0x00000100,
  REFR_HiddenFromLocalMap2 = 0x00000200,
  ACHR_StartsDead = 0x00000200,
  REFR_MotionBlurCastsShadows = 0x00000200,
  QuestItem = 0x00000400,
  PersistentReference = 0x00000400,
  LSCR_DisplaysInMainMenu = 0x00000400,
  InitiallyDisabled = 0x00000800,
  Ignored = 0x00001000,
  VisibleWhenDistant = 0x00008000,
  ACTI_RandomAnimationStart = 0x00010000,
  ACTI_Dangerous = 0x00020000,
  CELL_InteriorOffLimits = 0x00020000,
  Compressed = 0x00040000,
  CantWait = 0x00080000,
  ACTI_IgnoreObjectInteraction = 0x00100000,
  IsMarker = 0x00800000,
  ACTI_Obstacle = 0x02000000,
  REFR_NoAIAcquire = 0x02000000,
  NavMeshGenFilter = 0x04000000,
  NavMeshGenBoundingBox = 0x08000000,
  FURN_MustExitToTalk = 0x10000000,
  REFR_ReflectedByAutoWater = 0x10000000,
  FURN_IDLM_ChildCanUse = 0x20000000,
  REFR_DontHavokSettle = 0x20000000,
  NavMeshGenGround = 0x40000000,
  REFR_NoRespawn = 0x40000000,
  REFR_BultiBound = 0x80000000,
};

namespace espm {
struct GroupDataInternal
{
  // Records and GRUPs
  // Pointing to type (record type or "GRUP"), it's RecordHeader/GroupHeader -
  // 4 bytes
  std::vector<void*> subs;
};
}

bool espm::GroupHeader::GetXY(int16_t& outX, int16_t& outY) const noexcept
{
  if (grType == GroupType::EXTERIOR_CELL_BLOCK ||
      grType == GroupType::EXTERIOR_CELL_SUBBLOCK) {
    outY = *(int16_t*)label;
    outX = *(int16_t*)(label + 2);
    return true;
  }
  return false;
}

const char* espm::GroupHeader::GetRecordsType() const noexcept
{
  if (grType != GroupType::TOP)
    return nullptr;
  return label;
}

bool espm::GroupHeader::GetBlockNumber(int32_t& outBlockNum) const noexcept
{
  if (grType != GroupType::INTERIOR_CELL_BLOCK)
    return false;
  outBlockNum = *(int32_t*)label;
  return true;
}

bool espm::GroupHeader::GetSubBlockNumber(int32_t& outSubBlockNum) const
  noexcept
{
  if (grType != GroupType::INTERIOR_CELL_SUBBLOCK)
    return false;
  outSubBlockNum = *(int32_t*)label;
  return true;
}

bool espm::GroupHeader::GetParentWRLD(uint32_t& outId) const noexcept
{
  if (grType != GroupType::WORLD_CHILDREN)
    return false;
  outId = *(uint32_t*)label;
  return true;
}

bool espm::GroupHeader::GetParentCELL(uint32_t& outId) const noexcept
{
  if (grType != GroupType::CELL_CHILDREN &&
      grType != GroupType::CELL_PERSISTENT_CHILDREN &&
      grType != GroupType::CELL_TEMPORARY_CHILDREN &&
      grType != GroupType::CELL_VISIBLE_DISTANT_CHILDREN) {
    return false;
  }
  outId = *(uint32_t*)label;
  return true;
}

bool espm::GroupHeader::GetParentDIAL(uint32_t& outId) const noexcept
{
  if (grType != GroupType::TOPIC_CHILDREN)
    return false;
  outId = *(uint32_t*)label;
  return true;
}

void espm::GroupHeader::ForEachRecord(const RecordVisitor& f) const noexcept
{
  auto grData = (GroupDataInternal*)GroupDataPtrStorage();
  for (void* sub : grData->subs) {
    if (!memcmp(sub, "GRUP", 4))
      continue; // It's group, skipping
    if (f((espm::RecordHeader*)((int8_t*)sub + 8)))
      break;
  }
}

uint32_t espm::GetMappedId(uint32_t id,
                           const espm::IdMapping& mapping) noexcept
{
  const uint32_t shortId = id % 0x01000000;
  const uint8_t index = id / 0x01000000;
  return shortId + (mapping[index] * 0x01000000);
}

uint32_t espm::RecordHeader::GetId() const noexcept
{
  return id;
}

class espm::RecordHeaderAccess
{
public:
  template <class T>
  static void IterateFields(
    const espm::RecordHeader* rec, const T& f,
    espm::CompressedFieldsCache* compressedFieldsCache = nullptr)
  {
    const int8_t* ptr = ((int8_t*)rec) + sizeof(*rec);
    const int8_t* endPtr = ptr + rec->GetFieldsSizeSum();
    uint32_t fiDataSizeOverride = 0;

    if (rec->flags & RecordFlags::Compressed) {

      if (!compressedFieldsCache) {
        assert(
          0 &&
          "CompressedFieldsCache is required to iterate through compressed "
          "fields");
        return;
      }

      auto& decompressedFieldsHolder =
        compressedFieldsCache->pImpl->data[rec].decompressedFieldsHolder;
      if (!decompressedFieldsHolder) {

        const uint32_t* decompSize = reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        std::shared_ptr<std::vector<uint8_t>> out(new std::vector<uint8_t>);
        out->resize(*decompSize);
        try {
          const auto inSize = rec->GetFieldsSizeSum() - sizeof(uint32_t);
          ZlibDecompress(ptr, inSize, out->data(), out->size());
        } catch (std ::exception& e) {
          assert(0 && "ZlibDecompress has thrown an error");
          return;
        }

        decompressedFieldsHolder = out;
      }

      ptr = reinterpret_cast<int8_t*>(decompressedFieldsHolder->data());
      endPtr = reinterpret_cast<int8_t*>(decompressedFieldsHolder->data() +
                                         decompressedFieldsHolder->size());
    }

    while (ptr < endPtr) {
      const auto fiHeader = (FieldHeader*)ptr;
      ptr += sizeof(FieldHeader);
      const uint32_t fiDataSize =
        fiHeader->dataSize ? fiHeader->dataSize : fiDataSizeOverride;
      const char* fiData = (char*)ptr;
      ptr += fiDataSize;

      if (!memcmp(fiHeader->type, "XXXX", 4)) {
        fiDataSizeOverride = *(uint32_t*)fiData;
      }
      f(fiHeader->type, fiDataSize, fiData);
    }
  }
};

const char* espm::RecordHeader::GetEditorId(
  espm::CompressedFieldsCache* compressedFieldsCache) const noexcept
{
  const char* result = "";
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "EDID", 4))
        result = data;
    },
    compressedFieldsCache);
  return result;
}

espm::Type espm::RecordHeader::GetType() const noexcept
{
  return ((char*)this) - 8;
}

const espm::GroupStack& espm::RecordHeader::GetParentGroups() const noexcept
{
  return *(espm::GroupStack*)GroupStackPtrStorage();
}

uint32_t espm::RecordHeader::GetFlags() const noexcept
{
  return flags;
}

uint32_t espm::RecordHeader::GetFieldsSizeSum() const noexcept
{
  const auto ptr = ((int8_t*)this) - 4;
  return *(uint32_t*)ptr;
}

struct espm::Browser::Impl
{
  Impl() { objectReferences.reserve(100'000); }

  char* buf;
  size_t length;

  size_t pos = 0;
  uint32_t fiDataSizeOverride = 0;
  spp::sparse_hash_map<uint32_t, RecordHeader*> recById;
  spp::sparse_hash_map<uint64_t, std::vector<RecordHeader*>> navmeshes;
  std::vector<RecordHeader*> objectReferences;

  GroupStack grStack;
  std::vector<std::unique_ptr<GroupStack>> grStackCopies;
  std::vector<std::unique_ptr<GroupDataInternal>> grDataHolder;

  CompressedFieldsCache dummyCache;
};

espm::Browser::Browser(void* fileContent, size_t length)
  : pImpl(new Impl)
{
  pImpl->buf = (char*)fileContent;
  pImpl->length = length;
  while (ReadAny(nullptr))
    ;
  pImpl->dummyCache.pImpl->data.clear();
}

espm::Browser::~Browser()
{
  delete pImpl;
}

espm::RecordHeader* espm::Browser::LookupById(uint32_t formId) const noexcept
{
  auto it = pImpl->recById.find(formId);
  if (it == pImpl->recById.end())
    return nullptr;
  return it->second;
}

std::pair<espm::RecordHeader**, size_t> espm::Browser::FindNavMeshes(
  uint32_t worldSpaceId, espm::CellOrGridPos cellOrGridPos) const noexcept
{
  try {
    auto& vec = pImpl->navmeshes.at(NavMeshKey(worldSpaceId, cellOrGridPos));
    return { vec.data(), vec.size() };
  } catch (...) {
    return { nullptr, 0 };
  }
}

const std::vector<espm::RecordHeader*>& espm::Browser::GetRecordsByType(
  const char* type) const
{
  if (!strcmp(type, "REFR")) {
    return pImpl->objectReferences;
  }
  throw std::runtime_error(
    "GetRecordsByType currently supports only REFR records");
}

bool espm::Browser::ReadAny(void* parentGrStack)
{
  if (pImpl->pos >= pImpl->length)
    return false;

  char* pType = pImpl->buf + pImpl->pos;
  pImpl->pos += 4;
  uint32_t* pDataSize = (uint32_t*)(pImpl->buf + pImpl->pos);
  pImpl->pos += 4;

  const bool isGrup = !memcmp(pType, "GRUP", 4);
  if (isGrup) {
    // Read group header
    const auto grHeader = (GroupHeader*)(pImpl->buf + pImpl->pos);

    auto grData = new GroupDataInternal;
    pImpl->grDataHolder.emplace_back(grData);
    grHeader->GroupDataPtrStorage() = (uint64_t)grData;

    pImpl->pos += sizeof(GroupHeader);
    const size_t end = pImpl->pos + *pDataSize - 24;

    pImpl->grStack.push_back(grHeader);
    auto p = new GroupStack(pImpl->grStack);
    pImpl->grStackCopies.emplace_back(p);
    while (pImpl->pos < end) {
      auto nextSub = &pImpl->buf[pImpl->pos];
      if (ReadAny(p)) {
        grData->subs.push_back(nextSub);
      }
    }
    pImpl->grStack.pop_back();
  } else {
    // Read record header
    const auto recHeader = (RecordHeader*)(pImpl->buf + pImpl->pos);
    recHeader->GroupStackPtrStorage() = (uint64_t)parentGrStack;

    pImpl->recById[recHeader->id] = recHeader;

    if (recHeader->GetType() == "REFR")
      pImpl->objectReferences.push_back(recHeader);

    if (recHeader->GetType() == "NAVM") {
      auto nvnm = reinterpret_cast<NAVM*>(recHeader);

      auto& v = pImpl->navmeshes[NavMeshKey(
        nvnm->GetData(pImpl->dummyCache).worldSpaceId,
        nvnm->GetData(pImpl->dummyCache).cellOrGridPos)];
      v.push_back(nvnm);
    }

    pImpl->pos += sizeof(RecordHeader) + *pDataSize;
  }
  return true;
}

espm::TES4::Data espm::TES4::GetData() const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this, [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "HEDR", 4))
        result.header = (Header*)data;
      else if (!memcmp(type, "CNAM", 4))
        result.author = data;
      else if (!memcmp(type, "SNAM", 4))
        result.description = data;
      else if (!memcmp(type, "MAST", 4))
        result.masters.push_back(data);
    });
  return result;
}

espm::REFR::Data espm::REFR::GetData() const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this, [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "NAME", 4))
        result.baseId = *(uint32_t*)data;
      else if (!memcmp(type, "XSCL", 4))
        result.scale = *(float*)data;
      else if (!memcmp(type, "DATA", 4))
        result.loc = (LocationalData*)data;
    });
  return result;
}

espm::CONT::Data espm::CONT::GetData() const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this, [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "EDID", 4))
        result.editorId = data;
      else if (!memcmp(type, "FULL", 4))
        result.fullName = data;
      else if (!memcmp(type, "CNTO", 4))
        result.objects.push_back(*(ContainerObject*)data);
      else if (!memcmp(type, "COED", 4)) {
        // Not supported
      }
    });
  return result;
}

espm::TREE::Data espm::TREE::GetData() const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this, [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "EDID", 4))
        result.editorId = data;
      else if (!memcmp(type, "FULL", 4))
        result.fullName = data;
      else if (!memcmp(type, "OBND", 4))
        result.bounds = reinterpret_cast<const ObjectBounds*>(data);
      else if (!memcmp(type, "PFIG", 4))
        result.resultItem = *reinterpret_cast<const uint32_t*>(data);
      else if (!memcmp(type, "SNAM", 4))
        result.useSound = *reinterpret_cast<const uint32_t*>(data);
    });
  return result;
}

espm::FLOR::Data espm::FLOR::GetData() const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this, [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "EDID", 4))
        result.editorId = data;
      else if (!memcmp(type, "FULL", 4))
        result.fullName = data;
      else if (!memcmp(type, "OBND", 4))
        result.bounds = reinterpret_cast<const ObjectBounds*>(data);
      else if (!memcmp(type, "PFIG", 4))
        result.resultItem = *reinterpret_cast<const uint32_t*>(data);
      else if (!memcmp(type, "SNAM", 4))
        result.useSound = *reinterpret_cast<const uint32_t*>(data);
    });
  return result;
}

espm::LVLI::Data espm::LVLI::GetData() const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this, [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "EDID", 4))
        result.editorId = data;
      else if (!memcmp(type, "LVLF", 4))
        result.leveledItemFlags = *(uint8_t*)data;
      else if (!memcmp(type, "LVLG", 4))
        result.chanceNoneGlobalId = *(uint32_t*)data;
      else if (!memcmp(type, "LVLD", 4))
        result.chanceNone = *(uint8_t*)data;
      else if (!memcmp(type, "LLCT", 4)) {
        result.numEntries = *(uint8_t*)data;
        result.entries = (Entry*)(data + 1);
      }
    });
  return result;
}

espm::NAVM::Vertices::Vertices(const void* nvnmField_)
  : nvnmField(nvnmField_)
{
  this->numVerticesPtr = reinterpret_cast<const int32_t*>(
    reinterpret_cast<const uint8_t*>(nvnmField) + 16);

  this->beginPtr = reinterpret_cast<const std::array<float, 3>*>(
    reinterpret_cast<const uint8_t*>(nvnmField) + 20);
}

const std::array<float, 3>* espm::NAVM::Vertices::begin() const noexcept
{
  return this->beginPtr;
}

const std::array<float, 3>* espm::NAVM::Vertices::end() const noexcept
{
  return this->beginPtr + *this->numVerticesPtr;
}

espm::NAVM::Data espm::NAVM::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "NVNM", 4)) {
        result.worldSpaceId = *reinterpret_cast<const uint32_t*>(
          (reinterpret_cast<const uint8_t*>(data) + 8));
        result.cellOrGridPos = *reinterpret_cast<const CellOrGridPos*>(
          (reinterpret_cast<const uint8_t*>(data) + 12));
        result.vertices.reset(new Vertices(data));
      }
    },
    &compressedFieldsCache);
  return result;
}