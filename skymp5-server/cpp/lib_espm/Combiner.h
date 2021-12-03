#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include "GroupUtils.h"
#include "espm.h"

namespace espm {
class Browser;
class RecordHeader;
class Combiner;
class CombineBrowser;

struct BrowserInfo
{
  BrowserInfo() = default;
  BrowserInfo(const CombineBrowser* parent_, uint8_t fileIdx_)
    : parent(parent_)
    , fileIdx(fileIdx_){};

  // Returns 0 for empty (default constructed) LookupResult
  uint32_t ToGlobalId(uint32_t rawId) const noexcept;

  const CombineBrowser* const parent = nullptr;
  const uint8_t fileIdx = 0;
};

struct LookupResult : public BrowserInfo
{
  LookupResult() = default;
  LookupResult(const CombineBrowser* parent_, RecordHeader* rec_,
               uint8_t fileIdx_)
    : BrowserInfo(parent_, fileIdx_)
    , rec(rec_)
  {
  }

  RecordHeader* const rec = nullptr;
};

class CombineBrowser
{
  friend class espm::Combiner;

public:
  // Returns default constructed LookupResult on failure
  // Gets record from the last file in the load order
  LookupResult LookupById(uint32_t formId) const noexcept;

  // Returns a record for each file adding/editing record with such id
  std::vector<LookupResult> LookupByIdAll(uint32_t formId) const noexcept;

  std::pair<espm::RecordHeader**, size_t> FindNavMeshes(
    uint32_t worldSpaceId, espm::CellOrGridPos cellOrGridPos) const noexcept;

  std::vector<const std::vector<espm::RecordHeader*>*> GetRecordsByType(
    const char* type) const;

  std::vector<const std::vector<espm::RecordHeader*>*> GetRecordsAtPos(
    uint32_t cellOrWorld, int16_t cellX, int16_t cellY) const;

  // Returns nullptr on failure
  const espm::IdMapping* GetCombMapping(size_t fileIndex) const noexcept;
  const espm::IdMapping* GetRawMapping(size_t fileIndex) const noexcept;

  // CompressedFieldsCache is not logically related to Combiner, this method is
  // added for usability
  espm::CompressedFieldsCache& GetCache() const noexcept;

  const GroupStack& GetParentGroupsEnsured(const RecordHeader* rec) const;
  const std::vector<void*>& GetSubsEnsured(const GroupHeader* group) const;

private:
  struct Impl;
  Impl* pImpl;

  CombineBrowser() = default;
  CombineBrowser(const CombineBrowser&) = delete;
  void operator=(const CombineBrowser&) = delete;
};

class Combiner
{
public:
  Combiner();
  ~Combiner();

  class CombineError : public std::logic_error
  {
  public:
    CombineError(const std::string& str)
      : logic_error(str){};
  };

  void AddSource(Browser* src, const char* fileName) noexcept;

  // Throws CombineError
  std::unique_ptr<CombineBrowser> Combine();

private:
  CombineBrowser::Impl* pImpl;

  Combiner(const Combiner&) = delete;
  void operator=(const Combiner&) = delete;
};

}
