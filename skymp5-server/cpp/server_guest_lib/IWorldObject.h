#pragma once

class NiPoint3;

class IWorldObject
{
public:
  virtual ~IWorldObject() = default;

  virtual const NiPoint3& GetPos() const = 0;
  virtual const NiPoint3& GetAngle() const = 0;
  virtual const uint32_t& GetCellOrWorld() const = 0;
};
