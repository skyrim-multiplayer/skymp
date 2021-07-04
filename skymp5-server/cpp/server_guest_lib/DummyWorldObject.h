#pragma once
#include "IWorldObject.h"
#include "NiPoint3.h"
#include <cstdint>

class DummyWorldObject : public IWorldObject
{
public:
  DummyWorldObject(NiPoint3 pos_, NiPoint3 angle_, uint32_t cellOrWorld_)
    : pos(pos_)
    , angle(angle_)
    , cellOrWorld(cellOrWorld_)
  {
  }

  const NiPoint3& GetPos() const override { return pos; }
  const NiPoint3& GetAngle() const override { return angle; }
  const uint32_t& GetCellOrWorld() const override { return cellOrWorld; }

private:
  NiPoint3 pos, angle;
  uint32_t cellOrWorld;
};