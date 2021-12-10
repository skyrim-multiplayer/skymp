#pragma once
#include "FormDesc.h"
#include "IWorldObject.h"
#include "NiPoint3.h"
#include <cstdint>

class DummyWorldObject : public IWorldObject
{
public:
  DummyWorldObject(NiPoint3 pos_, NiPoint3 angle_, FormDesc cellOrWorld_)
    : pos(pos_)
    , angle(angle_)
    , cellOrWorld(cellOrWorld_)
  {
  }

  const NiPoint3& GetPos() const override { return pos; }
  const NiPoint3& GetAngle() const override { return angle; }
  const FormDesc& GetCellOrWorld() const override { return cellOrWorld; }

private:
  NiPoint3 pos, angle;
  FormDesc cellOrWorld;
};
