#include "GridService.h"

#include "MpObjectReference.h"
#include "PartOne.h"
#include "WorldState.h"
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

GridService::GridService(PartOne& partOne_)
  : partOne(partOne_)
{
}

GridService::GridInfo& GridService::GetOrCreateGridInfo(uint32_t cellOrWorld)
{
  return grids[cellOrWorld];
}

const std::set<MpObjectReference*>& GridService::GetNeighborsByPosition(
  uint32_t cellOrWorld, int16_t cellX, int16_t cellY)
{
  auto& gridInfo = GetOrCreateGridInfo(cellOrWorld);
  return gridInfo.grid->GetNeighboursByPosition(cellX, cellY);
}

GridDiff<MpObjectReference*> GridService::MoveObjectReference(
  MpObjectReference* objRef, uint32_t cellOrWorld, int16_t x, int16_t y)
{
  auto& gridInfo = GetOrCreateGridInfo(cellOrWorld);
  auto gridDiff = gridInfo.grid->MoveWithDiff(objRef, x, y);

  std::vector<std::string> added;
  for (auto ptr : gridDiff.added) {
    std::stringstream ss;
    ss << std::hex << ptr->GetFormId();
    added.push_back(ss.str());
  }

  std::vector<std::string> removed;
  for (auto ptr : gridDiff.removed) {
    std::stringstream ss;
    ss << std::hex << ptr->GetFormId();
    removed.push_back(ss.str());
  }

  spdlog::warn("!!! GridService::MoveObjectReference objRef={:x} "
               "cellOrWorld={:x} x={} y={} DIFF_ADDED=[{}] DIFF_REMOVED=[{}]",
               objRef->GetFormId(), cellOrWorld, x, y, fmt::join(added, ","),
               fmt::join(removed, ","));

  return gridDiff;
}

void GridService::ForgetObjectReference(MpObjectReference* objRef,
                                        uint32_t cellOrWorld)
{
  auto it = grids.find(cellOrWorld);
  if (it != grids.end()) {
    it->second.grid->Forget(objRef);
  }
}

bool GridService::IsChunkLoaded(uint32_t cellOrWorld, int16_t x,
                                int16_t y) const
{
  auto it = grids.find(cellOrWorld);
  if (it == grids.end()) {
    return false;
  }

  auto xIt = it->second.loadedChunks.find(x);
  if (xIt == it->second.loadedChunks.end()) {
    return false;
  }

  auto yIt = xIt->second.find(y);
  return yIt != xIt->second.end() && yIt->second;
}

void GridService::SetChunkLoaded(uint32_t cellOrWorld, int16_t x, int16_t y,
                                 bool loaded)
{
  grids[cellOrWorld].loadedChunks[x][y] = loaded;
}

void GridService::Clear()
{
  grids.clear();
}