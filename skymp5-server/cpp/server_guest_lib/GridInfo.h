#pragma once
#include "Grid.h"
#include <map>
#include <memory>

class MpObjectReference;

struct GridInfo
{
  std::shared_ptr<GridImpl<MpObjectReference*>> grid =
    std::make_shared<GridImpl<MpObjectReference*>>();
  std::map<int16_t, std::map<int16_t, bool>> loadedChunks;
};
