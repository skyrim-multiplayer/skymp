#pragma once
#include "CellOrGridPos.h"
#include "GroupHeader.h"
#include "GroupStack.h"
#include "RecordHeader.h"
#include <memory>
#include <utility>
#include <vector>

#pragma pack(push, 1)

namespace espm {

class Browser
{
public:
  Browser(const void* fileContent, size_t length);
  ~Browser();

  const RecordHeader* LookupById(uint32_t formId) const noexcept;
  std::pair<const RecordHeader**, size_t> FindNavMeshes(
    uint32_t worldSpaceId, CellOrGridPos cellOrGridPos) const noexcept;
  const std::vector<const RecordHeader*>& GetRecordsByType(
    const char* type) const;
  const std::vector<const RecordHeader*>& GetRecordsAtPos(uint32_t cellOrWorld,
                                                          int16_t cellX,
                                                          int16_t cellY) const;
  const GroupStack* GetParentGroupsOptional(const RecordHeader* rec) const;
  const GroupStack& GetParentGroupsEnsured(const RecordHeader* rec) const;
  const std::vector<const void*>* GetSubsOptional(
    const GroupHeader* group) const;
  const std::vector<const void*>& GetSubsEnsured(
    const GroupHeader* group) const;

private:
  bool ReadAny(const GroupStack* parentGrStack);
  Browser(const Browser&) = delete;
  void operator=(const Browser&) = delete;

private:
  struct Impl;
  std::unique_ptr<Impl> pImpl;
};

}

#pragma pack(pop)
