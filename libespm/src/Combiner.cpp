#include "libespm/Combiner.h"
#include "libespm/espm.h"
#include <array>
#include <fmt/format.h>
#include <sparsepp/spp.h>
#include <string>

struct Source
{
  espm::Browser* br = nullptr;
  std::string fileName;
  std::unique_ptr<espm::IdMapping> toComb, toRaw;
};

struct espm::CombineBrowser::Impl
{
  espm::CompressedFieldsCache cache;

  std::array<Source, 256> sources;
  size_t numSources = 0;

  // returns index of sources array or -1 if not found
  int GetFileIndex(const char* fileName) const noexcept
  {
    if (fileName[0] != '\0') {
      for (int i = 0; i < (int)sources.size(); ++i) {
        if (sources[i].fileName == fileName)
          return i;
      }
    }
    return -1;
  }
};

espm::Combiner::Combiner()
  : pImpl(nullptr)
{
  pImpl = new CombineBrowser::Impl;
}

espm::Combiner::~Combiner()
{
  delete pImpl;
}

void espm::Combiner::AddSource(Browser* src, const char* fileName) noexcept
{
  if (pImpl->numSources >= std::size(pImpl->sources)) {
    ++pImpl->numSources;
    return;
  }
  pImpl->sources[pImpl->numSources++] = { src, fileName, nullptr };
}

std::unique_ptr<espm::CombineBrowser> espm::Combiner::Combine()
{
  if (pImpl->numSources > std::size(pImpl->sources)) {
    throw CombineError("too many sources");
  }

  for (size_t i = 0; i < pImpl->numSources; ++i) {
    auto& src = pImpl->sources[i];
    if (!src.br) {
      throw CombineError("nullptr source with index " + std::to_string(i));
    }

    const auto tes4 = espm::Convert<espm::TES4>(src.br->LookupById(0));
    if (!tes4) {
      throw CombineError(src.fileName + " doesn't have TES4 record");
    }
    espm::CompressedFieldsCache dummyCache;
    const auto masters = tes4->GetData(dummyCache).masters;

    auto toComb = std::make_unique<IdMapping>();
    toComb->fill(0xff);
    auto toRaw = std::make_unique<IdMapping>();
    toRaw->fill(0xff);
    size_t m = 0;
    for (m = 0; m < masters.size(); ++m) {
      const int globalIdx = pImpl->GetFileIndex(masters[m]);
      if (globalIdx == -1) {
        throw CombineError(src.fileName + " has unresolved dependency (" +
                           masters[m] + ")");
      }
      (*toComb)[m] = (uint8_t)globalIdx;
      (*toRaw)[globalIdx] = (uint8_t)m;
    }
    (*toComb)[m] = (uint8_t)i;
    (*toRaw)[i] = (uint8_t)m;
    src.toComb = std::move(toComb);
    src.toRaw = std::move(toRaw);
  }

  std::unique_ptr<espm::CombineBrowser> res(new espm::CombineBrowser);
  res->pImpl = pImpl;
  return res;
}

uint32_t espm::BrowserInfo::ToGlobalId(uint32_t rawId) const noexcept
{
  if (!parent)
    return 0;
  const auto mapping = parent->GetCombMapping(fileIdx);
  return espm::GetMappedId(rawId, *mapping);
}

espm::LookupResult espm::CombineBrowser::LookupById(
  uint32_t combFormId) const noexcept
{
  RecordHeader* resRec = nullptr;
  uint8_t resFileIdx = 0;
  for (size_t i = 0; i < pImpl->numSources; ++i) {
    auto& src = pImpl->sources[i];
    const uint32_t rawFormId = espm::GetMappedId(combFormId, *src.toRaw);
    if (rawFormId >= 0xff000000)
      continue;
    auto rec = src.br->LookupById(rawFormId);
    if (rec) {
      resRec = rec;
      resFileIdx = (uint8_t)i;
    }
  }
  return resRec ? LookupResult(this, resRec, resFileIdx) : LookupResult();
}

std::vector<espm::LookupResult> espm::CombineBrowser::LookupByIdAll(
  uint32_t combFormId) const noexcept
{
  std::vector<espm::LookupResult> res;
  for (size_t i = 0; i < pImpl->numSources; ++i) {
    auto& src = pImpl->sources[i];
    const uint32_t rawFormId = espm::GetMappedId(combFormId, *src.toRaw);
    if (rawFormId >= 0xff000000)
      continue;
    auto rec = src.br->LookupById(rawFormId);
    if (rec) {
      res.push_back({ this, rec, static_cast<uint8_t>(i) });
    }
  }
  return res;
}

std::pair<espm::RecordHeader**, size_t> espm::CombineBrowser::FindNavMeshes(
  uint32_t worldSpaceId, espm::CellOrGridPos cellOrGridPos) const noexcept
{
  for (size_t i = 0; i < pImpl->numSources; ++i) {
    auto& src = pImpl->sources[i];
    const uint32_t rawFormId = espm::GetMappedId(worldSpaceId, *src.toRaw);
    if (rawFormId >= 0xff000000)
      continue;
    auto p = src.br->FindNavMeshes(worldSpaceId, cellOrGridPos);
    auto front = p.first;
    auto size = p.second;
    if (front && *front) {
      return { front, size };
    }
  }
  return { nullptr, 0 };
}

std::vector<const std::vector<espm::RecordHeader*>*>
espm::CombineBrowser::GetRecordsByType(const char* type) const
{
  std::vector<const std::vector<espm::RecordHeader*>*> res;
  for (size_t i = 0; i < pImpl->numSources; ++i) {
    res.push_back(&pImpl->sources[i].br->GetRecordsByType(type));
  }
  return res;
}

std::vector<const std::vector<espm::RecordHeader*>*>
espm::CombineBrowser::GetRecordsAtPos(uint32_t cellOrWorld, int16_t cellX,
                                      int16_t cellY) const
{
  std::vector<const std::vector<espm::RecordHeader*>*> res;
  for (size_t i = 0; i < pImpl->numSources; ++i) {
    res.push_back(
      &pImpl->sources[i].br->GetRecordsAtPos(cellOrWorld, cellX, cellY));
  }
  return res;
}

const espm::IdMapping* espm::CombineBrowser::GetCombMapping(
  size_t fileIndex) const noexcept
{
  if (fileIndex >= pImpl->numSources)
    return nullptr;
  return pImpl->sources[fileIndex].toComb.get();
}

const espm::IdMapping* espm::CombineBrowser::GetRawMapping(
  size_t fileIndex) const noexcept
{
  if (fileIndex >= pImpl->numSources)
    return nullptr;
  return pImpl->sources[fileIndex].toRaw.get();
}

espm::CompressedFieldsCache& espm::CombineBrowser::GetCache() const noexcept
{
  return pImpl->cache;
}

namespace espm {

const GroupStack& CombineBrowser::GetParentGroupsEnsured(
  const RecordHeader* rec) const
{
  for (size_t i = 0; i < pImpl->numSources; ++i) {
    const auto result = pImpl->sources[i].br->GetParentGroupsOptional(rec);
    if (result) {
      return *result;
    }
  }
  throw std::runtime_error(fmt::format(
    "espm::CombineBrowser: no browsers know record id={:#x}", rec->GetId()));
}

const std::vector<void*>& CombineBrowser::GetSubsEnsured(
  const GroupHeader* group) const
{
  for (size_t i = 0; i < pImpl->numSources; ++i) {
    const auto result = pImpl->sources[i].br->GetSubsOptional(group);
    if (result) {
      return *result;
    }
  }
  throw std::runtime_error(
    "espm::CombineBrowser: no browsers know requested group");
}

}
