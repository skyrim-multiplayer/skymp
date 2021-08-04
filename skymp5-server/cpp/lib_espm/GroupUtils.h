#pragma once

#include "espm.h"

namespace espm {

template <class TBrowser>
const GroupHeader* GetExteriorWorldGroup(const TBrowser& browser,
                                         const RecordHeader* rec)
{
  for (auto gr : browser.GetParentGroupsEnsured(rec)) {
    if (gr->GetGroupType() == GroupType::WORLD_CHILDREN) {
      return gr;
    }
  }
  return nullptr;
}

template <class TBrowser>
const GroupHeader* GetCellGroup(const TBrowser& browser,
                                const RecordHeader* rec)
{
  for (auto gr : browser.GetParentGroupsEnsured(rec)) {
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

template <class TBrowser>
uint32_t GetWorldOrCell(const TBrowser& browser, const RecordHeader* rec)
{
  const auto world = GetExteriorWorldGroup(browser, rec);
  const auto cell = GetCellGroup(browser, rec);

  uint32_t worldOrCell;

  if (!world || !world->GetParentWRLD(worldOrCell)) {
    worldOrCell = 0;
  }

  if (!worldOrCell) {
    if (!cell->GetParentCELL(worldOrCell)) {
      return 0;
    }
  }

  return worldOrCell;
}

using RecordVisitor = std::function<bool(const espm::RecordHeader*)>;

template <class TBrowser>
void ForEachChildRecord(const TBrowser& browser, const GroupHeader* group,
                        const RecordVisitor& f)
{
  for (const void* sub : browser.GetSubsEnsured(group)) {
    if (!memcmp(sub, "GRUP", 4)) {
      continue; // It's group, skipping
    }
    if (f(reinterpret_cast<const espm::RecordHeader*>(
          reinterpret_cast<const int8_t*>(sub) + 8))) {
      break;
    }
  }
}

}
