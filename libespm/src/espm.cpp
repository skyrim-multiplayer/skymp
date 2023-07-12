#include "libespm/ZlibUtils.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <memory>
#include <sparsepp/spp.h>

#include "libespm/GroupUtils.h"
#include "libespm/espm.h"

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

class RefrKey
{
public:
  RefrKey(uint32_t cellOrWorld, int16_t x, int16_t y)
    : v(InitValue(cellOrWorld, x, y))
  {
  }

  operator uint64_t() const noexcept { return v; }

private:
  static uint64_t InitValue(uint32_t cellOrWorld, int16_t x_, int16_t y_)
  {
    union
    {
      struct
      {
        int16_t x, y;
      };
      uint32_t unsignedInt;
    } myUnion;
    myUnion.x = x_;
    myUnion.y = y_;
    return MakeUInt64(cellOrWorld, myUnion.unsignedInt);
  }

  const uint64_t v;
};

const std::map<std::string, uint32_t> kCorrectHashcode{
  { "Skyrim.esm", 0xaf75991dUL },
  { "Update.esm", 0x17ab5e20UL },
  { "Dawnguard.esm", 0xcc81e5d8UL },
  { "HearthFires.esm", 0xbad9393aUL },
  { "Dragonborn.esm", 0xeb10e82UL }
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

uint32_t CalculateHashcode(const void* readBuffer, size_t length)
{
  return ZlibGetCRC32Checksum(readBuffer, length);
}

uint32_t GetCorrectHashcode(const std::string& fileName)
{
  auto iter = kCorrectHashcode.find(fileName);
  return iter == kCorrectHashcode.end() ? 0 : iter->second;
}
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
    outY = *reinterpret_cast<const int16_t*>(label);
    outX = *reinterpret_cast<const int16_t*>(label + 2);
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
  outBlockNum = *reinterpret_cast<const int32_t*>(label);
  return true;
}

bool espm::GroupHeader::GetSubBlockNumber(
  int32_t& outSubBlockNum) const noexcept
{
  if (grType != GroupType::INTERIOR_CELL_SUBBLOCK)
    return false;
  outSubBlockNum = *reinterpret_cast<const int32_t*>(label);
  return true;
}

bool espm::GroupHeader::GetParentWRLD(uint32_t& outId) const noexcept
{
  if (grType != GroupType::WORLD_CHILDREN)
    return false;
  outId = *reinterpret_cast<const uint32_t*>(label);
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
  outId = *reinterpret_cast<const uint32_t*>(label);
  return true;
}

bool espm::GroupHeader::GetParentDIAL(uint32_t& outId) const noexcept
{
  if (grType != GroupType::TOPIC_CHILDREN)
    return false;
  outId = *reinterpret_cast<const uint32_t*>(label);
  return true;
}

uint32_t espm::GroupHeader::GetGroupLabelAsUint() const noexcept
{
  return *reinterpret_cast<const uint32_t*>(label);
}

espm::GroupType espm::GroupHeader::GetGroupType() const noexcept
{
  return grType;
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
  static void IterateFields(const espm::RecordHeader* rec, const T& f,
                            espm::CompressedFieldsCache& compressedFieldsCache)
  {
    const int8_t* ptr = (reinterpret_cast<const int8_t*>(rec)) + sizeof(*rec);
    const int8_t* endPtr = ptr + rec->GetFieldsSizeSum();
    uint32_t fiDataSizeOverride = 0;

    if (rec->flags & RecordFlags::Compressed) {
      auto& decompressedFieldsHolder =
        compressedFieldsCache.pImpl->data[rec].decompressedFieldsHolder;
      if (!decompressedFieldsHolder) {
        const uint32_t* decompSize = reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        auto out = std::make_shared<std::vector<uint8_t>>();
        out->resize(*decompSize);

        const auto inSize = rec->GetFieldsSizeSum() - sizeof(uint32_t);
        ZlibDecompress(ptr, inSize, out->data(), out->size());

        decompressedFieldsHolder = out;
      }

      ptr = reinterpret_cast<int8_t*>(decompressedFieldsHolder->data());
      endPtr = reinterpret_cast<int8_t*>(decompressedFieldsHolder->data() +
                                         decompressedFieldsHolder->size());
    }

    while (ptr < endPtr) {
      const auto fiHeader = reinterpret_cast<const FieldHeader*>(ptr);
      ptr += sizeof(FieldHeader);
      const uint32_t fiDataSize =
        fiHeader->dataSize ? fiHeader->dataSize : fiDataSizeOverride;
      const char* fiData = reinterpret_cast<const char*>(ptr);
      ptr += fiDataSize;

      if (!memcmp(fiHeader->type, "XXXX", 4)) {
        fiDataSizeOverride = *reinterpret_cast<const uint32_t*>(fiData);
      }
      f(fiHeader->type, fiDataSize, fiData);
    }
  }
};

void espm::IterateFields_(const espm::RecordHeader* rec,
                          const espm::IterateFieldsCallback& f,
                          espm::CompressedFieldsCache& compressedFieldsCache)
{
  espm::RecordHeaderAccess::IterateFields(rec, f, compressedFieldsCache);
}

const char* espm::RecordHeader::GetEditorId(
  espm::CompressedFieldsCache& compressedFieldsCache) const noexcept
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

namespace {

std::wstring ReadWstring(const uint8_t* ptr)
{
  const uint16_t scriptNameSize = *reinterpret_cast<const uint16_t*>(ptr);
  const wchar_t* scriptName = reinterpret_cast<const wchar_t*>(ptr + 2);
  return std::wstring(scriptName, scriptNameSize / 2);
}

espm::PropertyType GetElementType(espm::PropertyType arrayType)
{
  return static_cast<espm::PropertyType>(static_cast<int>(arrayType) - 10);
}

const uint8_t* ReadPropertyValue(const uint8_t* p, espm::Property* prop,
                                 uint16_t objFormat)
{
  const auto t = prop->propertyType;
  switch (t) {
    case espm::PropertyType::Object:
      if (objFormat == 1)
        prop->value.formId = *reinterpret_cast<const uint32_t*>(p);
      else if (objFormat == 2)
        prop->value.formId = *reinterpret_cast<const uint32_t*>(p + 4);
      else
        throw std::runtime_error("Unknown objFormat (" +
                                 std::to_string(objFormat) + ")");
      return p + 8;
    case espm::PropertyType::Int:
      prop->value.integer = *reinterpret_cast<const int32_t*>(p);
      return p + 4;
    case espm::PropertyType::Float:
      prop->value.floatingPoint = *reinterpret_cast<const float*>(p);
      return p + 4;
    case espm::PropertyType::Bool:
      prop->value.boolean = *reinterpret_cast<const int8_t*>(p);
      return p + 1;
    case espm::PropertyType::String: {
      uint16_t length = *reinterpret_cast<const uint16_t*>(p);
      p += 2;
      prop->value.str = { reinterpret_cast<const char*>(p), length };
      p += length;
      return p;
    }
    case espm::PropertyType::ObjectArray:
    case espm::PropertyType::IntArray:
    case espm::PropertyType::FloatArray:
    case espm::PropertyType::BoolArray:
    case espm::PropertyType::StringArray: {
      uint32_t arrayLength = *reinterpret_cast<const uint32_t*>(p);
      p += 4;
      for (uint32_t i = 0; i < arrayLength; ++i) {
        espm::Property element;
        element.propertyType = GetElementType(t);
        p = ReadPropertyValue(p, &element, objFormat);
        prop->array.push_back(element.value);
      }
      return p;
    }
    default:
      throw std::runtime_error("Script properties with type " +
                               std::to_string(static_cast<int>(t)) +
                               " are not yet supported");
  }
}

void FillScriptArray(const uint8_t* p, std::vector<espm::Script>& out,
                     uint16_t objFormat)
{
  for (uint16_t i = 0; i < out.size(); ++i) {
    const uint16_t length = *reinterpret_cast<const uint16_t*>(p);
    p += 2;
    out[i].scriptName = std::string(reinterpret_cast<const char*>(p), length);
    p += length;
    out[i].status = *p;
    p++;
    const uint16_t numProperties = *reinterpret_cast<const uint16_t*>(p);
    p += 2;
    for (uint16_t j = 0; j < numProperties; ++j) {
      const uint16_t length = *reinterpret_cast<const uint16_t*>(p);
      p += 2;

      espm::Property prop;
      prop.propertyName =
        std::string(reinterpret_cast<const char*>(p), length);
      p += length;
      prop.propertyType = static_cast<espm::PropertyType>(*p);
      p++;
      prop.status = *p;
      p++;
      p = ReadPropertyValue(p, &prop, objFormat);
      out[i].properties.insert(prop);
    }
  }
}

} // namespace

void espm::RecordHeader::GetScriptData(
  ScriptData* out,
  espm::CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  ScriptData res;

  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "VMAD", 4)) {
        res.version = *reinterpret_cast<const uint16_t*>(data);
        res.objFormat = *reinterpret_cast<const uint16_t*>(
          (reinterpret_cast<const uint8_t*>(data) + 2));
        const uint16_t scriptCount = *reinterpret_cast<const uint16_t*>(
          (reinterpret_cast<const uint8_t*>(data) + 4));

        auto p = reinterpret_cast<const uint8_t*>(data) + 6;
        res.scripts.resize(scriptCount);
        FillScriptArray(p, res.scripts, res.objFormat);
      }
    },
    compressedFieldsCache);

  *out = res;
}

std::vector<uint32_t> espm::RecordHeader::GetKeywordIds(
  espm::CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  std::vector<uint32_t> res;
  uint32_t count = 0;

  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "KSIZ", 4)) {
        count = *reinterpret_cast<const uint32_t*>(data);
      }
      if (!memcmp(type, "KWDA", 4)) {
        for (uint32_t i = 0; i < count; ++i)
          res.push_back(reinterpret_cast<const uint32_t*>(data)[i]);
      }
    },
    compressedFieldsCache);

  return res;
}

espm::Type espm::RecordHeader::GetType() const noexcept
{
  // TODO(#1244): fix this
  return ((char*)this) - 8;
}

uint32_t espm::RecordHeader::GetFlags() const noexcept
{
  return flags;
}

uint32_t espm::RecordHeader::GetFieldsSizeSum() const noexcept
{
  const auto ptr = (reinterpret_cast<const int8_t*>(this)) - 4;
  return *reinterpret_cast<const uint32_t*>(ptr);
}

struct espm::Browser::Impl
{
  Impl() { objectReferences.reserve(100'000); }

  char* buf = nullptr;
  size_t length = 0;

  size_t pos = 0;
  uint32_t fiDataSizeOverride = 0;
  spp::sparse_hash_map<uint32_t, RecordHeader*> recById;
  spp::sparse_hash_map<uint64_t, std::vector<RecordHeader*>> navmeshes;
  spp::sparse_hash_map<uint64_t, std::vector<RecordHeader*>>
    cellOrWorldChildren;
  spp::sparse_hash_map<const GroupHeader*, const GroupDataInternal*>
    groupDataByGroupPtr;
  spp::sparse_hash_map<const RecordHeader*, const GroupStack*>
    groupStackByRecordPtr;
  std::vector<RecordHeader*> objectReferences;
  std::vector<RecordHeader*> constructibleObjects;
  std::vector<RecordHeader*> keywords;

  GroupStack grStack;
  std::vector<std::unique_ptr<GroupStack>> grStackCopies;
  std::vector<std::unique_ptr<GroupDataInternal>> grDataHolder;

  CompressedFieldsCache dummyCache;

  // may return null
  const GroupStack* GetParentGroupsOptional(const RecordHeader* rec) const
  {
    const auto it = groupStackByRecordPtr.find(rec);
    if (it == groupStackByRecordPtr.end()) {
      return nullptr;
    }
    return it->second;
  }

  // may return null
  const std::vector<void*>* GetSubsOptional(const GroupHeader* rec) const
  {
    const auto it = groupDataByGroupPtr.find(rec);
    if (it == groupDataByGroupPtr.end()) {
      return nullptr;
    }
    return &it->second->subs;
  }
};

espm::Browser::Browser(const void* fileContent, size_t length)
  : pImpl(new Impl)
{
  // TODO(#1244): buf should be const
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
  } catch (const std::out_of_range&) {
    return { nullptr, 0 };
  }
}

const std::vector<espm::RecordHeader*>& espm::Browser::GetRecordsByType(
  const char* type) const
{
  if (!strcmp(type, "REFR")) {
    return pImpl->objectReferences;
  }
  if (!strcmp(type, "COBJ")) {
    return pImpl->constructibleObjects;
  }
  if (!strcmp(type, "KYWD")) {
    return pImpl->keywords;
  }
  throw std::runtime_error(
    "GetRecordsByType currently supports only REFR and COBJ records");
}

const std::vector<espm::RecordHeader*>& espm::Browser::GetRecordsAtPos(
  uint32_t cellOrWorld, int16_t cellX, int16_t cellY) const
{
  const auto it =
    pImpl->cellOrWorldChildren.find(RefrKey(cellOrWorld, cellX, cellY));
  if (it == pImpl->cellOrWorldChildren.end()) {
    const static std::vector<espm::RecordHeader*> g_defaultValue{};
    return g_defaultValue;
  }
  return it->second;
}

namespace espm {

const GroupStack* Browser::GetParentGroupsOptional(
  const RecordHeader* rec) const
{
  return pImpl->GetParentGroupsOptional(rec);
}

const GroupStack& Browser::GetParentGroupsEnsured(
  const RecordHeader* rec) const
{
  const auto opt = GetParentGroupsOptional(rec);
  if (!opt) {
    throw std::runtime_error("espm::Browser: no parent groups for record");
  }
  return *opt;
}

const std::vector<void*>* Browser::GetSubsOptional(
  const GroupHeader* group) const
{
  return pImpl->GetSubsOptional(group);
}

const std::vector<void*>& Browser::GetSubsEnsured(
  const GroupHeader* group) const
{
  const auto opt = GetSubsOptional(group);
  if (!opt) {
    throw std::runtime_error("espm::Browser: no subs for record");
  }
  return *opt;
}

} // namespace espm

bool espm::Browser::ReadAny(const GroupStack* parentGrStack)
{
  if (pImpl->pos >= pImpl->length) {
    return false;
  }

  // TODO(#1244): pType should be const
  char* pType = pImpl->buf + pImpl->pos;
  pImpl->pos += 4;
  const uint32_t* pDataSize =
    reinterpret_cast<const uint32_t*>(pImpl->buf + pImpl->pos);
  pImpl->pos += 4;

  const bool isGrup = !memcmp(pType, "GRUP", 4);
  if (isGrup) {
    // TODO(#1244): should be reinterpret_cast<const GroupHeader*>
    // Read group header
    const auto grHeader = (GroupHeader*)(pImpl->buf + pImpl->pos);

    auto grData = new GroupDataInternal;
    pImpl->grDataHolder.emplace_back(grData);
    pImpl->groupDataByGroupPtr.emplace(grHeader, grData);

    pImpl->pos += sizeof(GroupHeader);
    const size_t end = pImpl->pos + *pDataSize - 24;

    // TODO(#1244): grStack should be const
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
    const auto recHeader =
      reinterpret_cast<RecordHeader*>(pImpl->buf + pImpl->pos);
    pImpl->groupStackByRecordPtr.emplace(recHeader, parentGrStack);

    pImpl->recById[recHeader->id] = recHeader;
    auto t = recHeader->GetType();

    if (t == "REFR" || t == "ACHR") {
      pImpl->objectReferences.push_back(recHeader);
      const auto refr = reinterpret_cast<REFR*>(recHeader);

      CompressedFieldsCache dummyCache;
      const auto data = refr->GetData(dummyCache);

      if (data.loc) {
        const int16_t x = static_cast<int16_t>(data.loc->pos[0] / 4096);
        const int16_t y = static_cast<int16_t>(data.loc->pos[1] / 4096);
        const auto cellOrWorld = GetWorldOrCell(*this, refr);
        const RefrKey refrKey(cellOrWorld, x, y);
        pImpl->cellOrWorldChildren[refrKey].push_back(refr);
      }
    }

    if (t == "COBJ")
      pImpl->constructibleObjects.push_back(recHeader);

    if (t == "KYWD")
      pImpl->keywords.push_back(recHeader);

    if (t == "NAVM") {
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

espm::TES4::Data espm::TES4::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "HEDR", 4))
        result.header = reinterpret_cast<const Header*>(data);
      else if (!memcmp(type, "CNAM", 4))
        result.author = data;
      else if (!memcmp(type, "SNAM", 4))
        result.description = data;
      else if (!memcmp(type, "MAST", 4))
        result.masters.push_back(data);
    },
    compressedFieldsCache);
  return result;
}

espm::REFR::Data espm::REFR::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "NAME", 4)) {
        result.baseId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!memcmp(type, "XSCL", 4)) {
        result.scale = *reinterpret_cast<const float*>(data);
      } else if (!memcmp(type, "DATA", 4)) {
        result.loc = reinterpret_cast<const LocationalData*>(data);
      } else if (!memcmp(type, "XTEL", 4)) {
        result.teleport = reinterpret_cast<const DoorTeleport*>(data);
      } else if (!memcmp(type, "XPRM", 4)) {
        result.boundsDiv2 = reinterpret_cast<const float*>(data);
      } else if (!memcmp(type, "XCNT", 4)) {
        result.count = *reinterpret_cast<const uint32_t*>(data);
      }
    },
    compressedFieldsCache);
  return result;
}

namespace {
std::vector<espm::CONT::ContainerObject> GetContainerObjects(
  const espm::RecordHeader* rec,
  espm::CompressedFieldsCache& compressedFieldsCache)
{
  std::vector<espm::CONT::ContainerObject> objects;
  espm::RecordHeaderAccess::IterateFields(
    rec,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "CNTO", 4)) {
        objects.push_back(
          *reinterpret_cast<const espm::CONT::ContainerObject*>(data));
      } else if (!memcmp(type, "COED", 4)) {
        // Not supported
      }
    },
    compressedFieldsCache);
  return objects;
}
}

espm::CONT::Data espm::CONT::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "EDID", 4))
        result.editorId = data;
      else if (!memcmp(type, "FULL", 4))
        result.fullName = data;
    },
    compressedFieldsCache);
  result.objects = GetContainerObjects(this, compressedFieldsCache);
  return result;
}

espm::TREE::Data espm::TREE::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
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
    },
    compressedFieldsCache);
  return result;
}

espm::FLOR::Data espm::FLOR::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
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
    },
    compressedFieldsCache);
  return result;
}

espm::LVLI::Data espm::LVLI::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "EDID", 4)) {
        result.editorId = data;
      } else if (!memcmp(type, "LVLF", 4)) {
        result.leveledItemFlags = *reinterpret_cast<const uint8_t*>(data);
      } else if (!memcmp(type, "LVLG", 4)) {
        result.chanceNoneGlobalId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!memcmp(type, "LVLD", 4)) {
        result.chanceNone = *reinterpret_cast<const uint8_t*>(data);
      } else if (!memcmp(type, "LLCT", 4)) {
        result.numEntries = *reinterpret_cast<const uint8_t*>(data);
        // TODO(#1244): entries should be const
        result.entries = (Entry*)(data + 1);
      }
    },
    compressedFieldsCache);
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
    compressedFieldsCache);
  return result;
}

espm::FLST::Data espm::FLST::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "LNAM", 4)) {
        const auto formId = *reinterpret_cast<const uint32_t*>(data);
        result.formIds.push_back(formId);
      }
    },
    compressedFieldsCache);
  std::reverse(result.formIds.begin(), result.formIds.end());
  return result;
}

espm::ACTI::Data espm::ACTI::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  GetScriptData(&result.scriptData, compressedFieldsCache);
  return result;
}

espm::COBJ::Data espm::COBJ::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "CNTO", 4)) {
        result.inputObjects.push_back(
          *reinterpret_cast<const InputObject*>(data));
      } else if (!memcmp(type, "CNAM", 4)) {
        const auto formId = *reinterpret_cast<const uint32_t*>(data);
        result.outputObjectFormId = formId;
      } else if (!memcmp(type, "BNAM", 4)) {
        const auto formId = *reinterpret_cast<const uint32_t*>(data);
        result.benchKeywordId = formId;
      } else if (!memcmp(type, "NAM1", 4)) {
        const auto count = *reinterpret_cast<const uint16_t*>(data);
        result.outputCount = count;
      }
    },
    compressedFieldsCache);
  return result;
}

espm::OTFT::Data espm::OTFT::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "INAM", 4)) {
        result.formIds = reinterpret_cast<const uint32_t*>(data);
        result.count = dataSize / sizeof(dataSize);
      }
    },
    compressedFieldsCache);
  return result;
}

espm::NPC_::Data espm::NPC_::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "DOFT", 4)) {
        result.defaultOutfitId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!memcmp(type, "SOFT", 4)) {
        result.sleepOutfitId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!memcmp(type, "SNAM", 4)) {
        uint32_t formId = *reinterpret_cast<const uint32_t*>(data);
        int8_t rank = *reinterpret_cast<const int8_t*>(data);

        result.factions.push_back({ formId, rank });
      } else if (!memcmp(type, "ACBS", 4)) {
        const uint32_t flags = *reinterpret_cast<const uint32_t*>(data);

        result.isEssential = !!(flags & 0x02);
        result.isProtected = !!(flags & 0x800);
        result.magickaOffset = *reinterpret_cast<const uint16_t*>(data + 4);
        result.staminaOffset = *reinterpret_cast<const uint16_t*>(data + 6);
        result.healthOffset = *reinterpret_cast<const uint16_t*>(data + 20);

      } else if (!memcmp(type, "RNAM", 4)) {
        result.race = *reinterpret_cast<const uint32_t*>(data);
      } else if (!memcmp(type, "OBND", 4)) {
        if (const auto objectBounds =
              reinterpret_cast<const ObjectBounds*>(data)) {
          result.objectBounds = *objectBounds;
        }
      } else if (!memcmp(type, "SPLO", 4)) {
        result.spells.emplace(*reinterpret_cast<const uint32_t*>(data));
      }
    },
    compressedFieldsCache);

  result.objects = GetContainerObjects(this, compressedFieldsCache);
  return result;
}

espm::WEAP::Data espm::WEAP::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "DATA", 4)) {
        result.weapData = reinterpret_cast<const WeapData*>(data);
      } else if (!memcmp(type, "DNAM", 4)) {
        result.weapDNAM = reinterpret_cast<const DNAM*>(data);
      }
    },
    compressedFieldsCache);
  return result;
}

namespace espm {
ARMO::Data ARMO::GetData(CompressedFieldsCache& compressedFieldsCache) const
{
  Data result;
  bool hasDNAM = false;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!memcmp(type, "EITM", 4)) {
        result.enchantmentFormId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!memcmp(type, "DATA", 4)) {
        result.baseValue = *reinterpret_cast<const uint32_t*>(data);
        result.weight = *reinterpret_cast<const float*>(data + 4);
      } else if (!memcmp(type, "DNAM", 4)) {
        hasDNAM = true;
        result.baseRatingX100 = *reinterpret_cast<const uint32_t*>(data);
      }
    },
    compressedFieldsCache);
  if (!hasDNAM) {
    throw std::runtime_error("bad record ARMO? DNAM was not found");
  }
  return result;
}
}

espm::RACE::Data espm::RACE::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!memcmp(type, "DATA", 4)) {
        result.startingHealth = *reinterpret_cast<const float*>(data + 36);
        result.startingMagicka = *reinterpret_cast<const float*>(data + 40);
        result.startingStamina = *reinterpret_cast<const float*>(data + 44);
        result.healRegen = *reinterpret_cast<const float*>(data + 84);
        result.magickaRegen = *reinterpret_cast<const float*>(data + 88);
        result.staminaRegen = *reinterpret_cast<const float*>(data + 92);
        result.unarmedDamage = *reinterpret_cast<const float*>(data + 96);
        result.unarmedReach = *reinterpret_cast<const float*>(data + 100);
      } else if (!memcmp(type, "SPLO", 4)) {
        result.spells.emplace(*reinterpret_cast<const uint32_t*>(data));
      }
    },
    compressedFieldsCache);
  return result;
}

espm::GMST::Data espm::GMST::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!memcmp(type, "DATA", 4)) {
        result.value = *reinterpret_cast<const float*>(data);
      }
    },
    compressedFieldsCache);
  return result;
}

espm::Effects::Effects(const RecordHeader* parent)
  : parent(parent)
{
}

espm::Effects::Data espm::Effects::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  if (!parent) {
    return Data();
  }

  Data result;
  uint32_t effectIndex = 0;
  bool orderFlag = true;
  bool isValid = true;

  espm::RecordHeaderAccess::IterateFields(
    parent,
    [&](const char* type, uint32_t size, const char* data) {
      if (!memcmp(type, "EFID", 4)) {
        isValid = orderFlag == true;
        result.effects.emplace_back();
        result.effects[effectIndex].effectId =
          *reinterpret_cast<const uint32_t*>(data);
        orderFlag = false;
      } else if (!memcmp(type, "EFIT", 4)) {
        isValid = orderFlag == false;
        Effect& eff = result.effects[effectIndex];
        eff.magnitude = *reinterpret_cast<const float*>(data);
        eff.areaOfEffect = *reinterpret_cast<const uint32_t*>(data + 4);
        eff.duration = *reinterpret_cast<const uint32_t*>(data + 8);
        effectIndex++;
        orderFlag = true;
      }
      if (!isValid) {
        auto name = parent->GetEditorId(compressedFieldsCache);
        throw std::runtime_error(
          fmt::format("Bad effect array for edid={}", name));
      }
    },
    compressedFieldsCache);
  return result;
}

espm::ENCH::Data espm::ENCH::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  result.effects = Effects(this).GetData(compressedFieldsCache).effects;
  return result;
}

espm::MGEF::Data espm::MGEF::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!memcmp(type, "DATA", 4)) {
        result.data.primaryAV =
          espm::ActorValue(*reinterpret_cast<const uint32_t*>(data + 0x44));
      }
    },
    compressedFieldsCache);

  return result;
}

espm::ALCH::Data espm::ALCH::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  result.effects = Effects(this).GetData(compressedFieldsCache).effects;
  return result;
}

espm::INGR::Data espm::INGR::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  result.effects = Effects(this).GetData(compressedFieldsCache).effects;
  return result;
}

bool espm::BOOK::Data::IsFlagSet(const Flags flag) const noexcept
{
  return (flags & flag) == flag;
}

espm::BOOK::Data espm::BOOK::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;

  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!memcmp(type, "DATA", 4)) {
        result.flags =
          static_cast<Flags>(*reinterpret_cast<const uint8_t*>(data));

        result.spellOrSkillFormId =
          *reinterpret_cast<const uint32_t*>(data + 0x4);
      }
    },
    compressedFieldsCache);

  return result;
}

espm::KYWD::Data espm::KYWD::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  espm::RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!memcmp(type, "EDID", 4)) {
        result.editorId = data;
      }
    },
    compressedFieldsCache);
  return result;
}
