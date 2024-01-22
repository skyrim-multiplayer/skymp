#include "libespm/Browser.h"
#include "libespm/ACHR.h"
#include "libespm/COBJ.h"
#include "libespm/CellOrGridPos.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/GroupDataInternal.h"
#include "libespm/GroupUtils.h"
#include "libespm/KYWD.h"
#include "libespm/NAVM.h"
#include "libespm/NavMeshKey.h"
#include "libespm/QUST.h"
#include "libespm/REFR.h"
#include "libespm/RecordHeader.h"
#include "libespm/RefrKey.h"
#include <cstring>
#include <optional>
#include <sparsepp/spp.h>
#include <vector>

namespace espm {

struct Browser::Impl
{

  Impl() { objectReferences.reserve(100'000); }

  const char* buf = nullptr;
  size_t length = 0;

  size_t pos = 0;
  uint32_t fiDataSizeOverride = 0;
  spp::sparse_hash_map<uint32_t, const RecordHeader*> recById;
  spp::sparse_hash_map<uint64_t, std::vector<const RecordHeader*>> navmeshes;
  spp::sparse_hash_map<uint64_t, std::vector<const RecordHeader*>>
    cellOrWorldChildren;
  spp::sparse_hash_map<const GroupHeader*, const GroupDataInternal*>
    groupDataByGroupPtr;
  spp::sparse_hash_map<const RecordHeader*, const GroupStack*>
    groupStackByRecordPtr;
  std::vector<const RecordHeader*> objectReferences;
  std::vector<const RecordHeader*> constructibleObjects;
  std::vector<const RecordHeader*> keywords;
  std::vector<const RecordHeader*> quests;

  GroupStack grStack;
  std::vector<std::unique_ptr<GroupStack>> grStackCopies;
  std::vector<std::unique_ptr<GroupDataInternal>> grDataHolder;

  CompressedFieldsCache dummyCache;

  // may return null
  const GroupStack* GetParentGroupsOptional(
    const RecordHeader* rec) const noexcept
  {
    const auto it = groupStackByRecordPtr.find(rec);
    if (it == groupStackByRecordPtr.end()) {
      return nullptr;
    }
    return it->second;
  }

  // may return null
  const std::vector<const void*>* GetSubsOptional(
    const GroupHeader* rec) const noexcept
  {
    const auto it = groupDataByGroupPtr.find(rec);
    if (it == groupDataByGroupPtr.end()) {
      return nullptr;
    }
    return &it->second->subs;
  }
};

Browser::Browser(const void* fileContent, size_t length)
  : pImpl(std::make_unique<Impl>())
{
  pImpl->buf = static_cast<const char*>(fileContent);
  pImpl->length = length;
  while (ReadAny(nullptr))
    ;
  pImpl->dummyCache.data.clear();
}

Browser::~Browser() = default;

const RecordHeader* Browser::LookupById(uint32_t formId) const noexcept
{
  auto it = pImpl->recById.find(formId);
  if (it == pImpl->recById.end()) {
    return nullptr;
  }
  return it->second;
}

std::pair<const RecordHeader**, size_t> Browser::FindNavMeshes(
  uint32_t worldSpaceId, CellOrGridPos cellOrGridPos) const noexcept
{
  try {
    auto& vec = pImpl->navmeshes.at(NavMeshKey(worldSpaceId, cellOrGridPos));
    return { vec.data(), vec.size() };
  } catch (const std::out_of_range&) {
    return { nullptr, 0 };
  }
}

const std::vector<const RecordHeader*>& Browser::GetRecordsByType(
  const char* type) const
{
  if (!std::strcmp(type, "REFR")) {
    return pImpl->objectReferences;
  }
  if (!std::strcmp(type, "COBJ")) {
    return pImpl->constructibleObjects;
  }
  if (!std::strcmp(type, "KYWD")) {
    return pImpl->keywords;
  }
  if (!std::strcmp(type, "QUST")) {
    return pImpl->quests;
  }
  throw std::runtime_error("GetRecordsByType currently supports only REFR, "
                           "COBJ, KYWD and QUST records");
}

const std::vector<const RecordHeader*>& Browser::GetRecordsAtPos(
  uint32_t cellOrWorld, int16_t cellX, int16_t cellY) const
{
  const auto it =
    pImpl->cellOrWorldChildren.find(RefrKey(cellOrWorld, cellX, cellY));
  if (it == pImpl->cellOrWorldChildren.end()) {
    const static std::vector<const RecordHeader*> g_defaultValue{};
    return g_defaultValue;
  }
  return it->second;
}

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

const std::vector<const void*>* Browser::GetSubsOptional(
  const GroupHeader* group) const
{
  return pImpl->GetSubsOptional(group);
}

const std::vector<const void*>& Browser::GetSubsEnsured(
  const GroupHeader* group) const
{
  const auto opt = GetSubsOptional(group);
  if (!opt) {
    throw std::runtime_error("espm::Browser: no subs for record");
  }
  return *opt;
}

bool Browser::ReadAny(const GroupStack* parentGrStack)
{
  if (pImpl->pos >= pImpl->length) {
    return false;
  }

  const char* pType = static_cast<const char*>(pImpl->buf + pImpl->pos);
  pImpl->pos += 4;
  const uint32_t* pDataSize =
    reinterpret_cast<const uint32_t*>(pImpl->buf + pImpl->pos);
  pImpl->pos += 4;

  const bool isGrup = !std::memcmp(pType, "GRUP", 4);
  if (isGrup) {
    // Read group header
    const auto grHeader =
      reinterpret_cast<const GroupHeader*>(pImpl->buf + pImpl->pos);

    const auto grData = new GroupDataInternal;
    pImpl->grDataHolder.emplace_back(grData);
    pImpl->groupDataByGroupPtr.emplace(grHeader, grData);

    pImpl->pos += sizeof(GroupHeader);
    const size_t end = pImpl->pos + *pDataSize - 24;

    pImpl->grStack.push_back(grHeader);
    auto p = new GroupStack(pImpl->grStack);
    pImpl->grStackCopies.emplace_back(p);
    while (pImpl->pos < end) {
      const char* nextSub = &pImpl->buf[pImpl->pos];
      if (ReadAny(p)) {
        grData->subs.push_back(nextSub);
      }
    }
    pImpl->grStack.pop_back();
  } else {
    // Read record header
    const auto recHeader =
      reinterpret_cast<const RecordHeader*>(pImpl->buf + pImpl->pos);
    pImpl->groupStackByRecordPtr.emplace(recHeader, parentGrStack);

    pImpl->recById[recHeader->id] = recHeader;

    Type t = recHeader->GetType();
    if (utils::Is<espm::REFR>(t) || utils::Is<espm::ACHR>(t)) {
      pImpl->objectReferences.push_back(recHeader);
      const auto refr = reinterpret_cast<const REFR*>(recHeader);

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

    if (utils::Is<espm::COBJ>(t)) {
      pImpl->constructibleObjects.push_back(recHeader);
    }

    if (utils::Is<espm::KYWD>(t)) {
      pImpl->keywords.push_back(recHeader);
    }

    if (utils::Is<espm::NAVM>(t)) {
      auto nvnm = reinterpret_cast<const NAVM*>(recHeader);

      auto& v = pImpl->navmeshes[NavMeshKey(
        nvnm->GetData(pImpl->dummyCache).worldSpaceId,
        nvnm->GetData(pImpl->dummyCache).cellOrGridPos)];
      v.push_back(nvnm);
    }

    if (utils::Is<espm::QUST>(t)) {
      pImpl->quests.push_back(recHeader);
    }

    pImpl->pos += sizeof(RecordHeader) + *pDataSize;
  }
  return true;
}

}
