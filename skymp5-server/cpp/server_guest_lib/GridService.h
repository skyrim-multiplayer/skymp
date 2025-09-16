#pragma once
#include "Grid.h"
#include "ServiceBase.h"
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

class MpObjectReference;
class PartOne;

class GridService : public ServiceBase<GridService>
{
public:
  explicit GridService(PartOne& partOne_);

  struct GridInfo
  {
    std::shared_ptr<GridImpl<MpObjectReference*>> grid =
      std::make_shared<GridImpl<MpObjectReference*>>();
    std::map<int16_t, std::map<int16_t, bool>> loadedChunks;
  };

  GridInfo& GetOrCreateGridInfo(uint32_t cellOrWorld);
  const std::set<MpObjectReference*>& GetNeighborsByPosition(
    uint32_t cellOrWorld, int16_t cellX, int16_t cellY);
  
  GridDiff<MpObjectReference*> MoveObjectReference(
    MpObjectReference* objRef, uint32_t cellOrWorld, int16_t x, int16_t y);
  
  void ForgetObjectReference(MpObjectReference* objRef, uint32_t cellOrWorld);
  
  bool IsChunkLoaded(uint32_t cellOrWorld, int16_t x, int16_t y) const;
  void SetChunkLoaded(uint32_t cellOrWorld, int16_t x, int16_t y, bool loaded);

  void Clear();

  auto& GetGrids() { return grids; }

private:
  PartOne& partOne;
  std::unordered_map<uint32_t, GridInfo> grids;
};