#pragma once

class NiPoint3;
class FormDesc;

class IWorldObject
{
public:
  virtual ~IWorldObject() = default;

  virtual const NiPoint3& GetPos() const = 0;
  virtual const NiPoint3& GetAngle() const = 0;
  virtual const FormDesc& GetCellOrWorld() const = 0;
};
