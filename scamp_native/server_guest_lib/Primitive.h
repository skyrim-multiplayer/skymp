#pragma once
#include "NiPoint3.h"
#include "espm.h"
#include <vector>

// Supports only Z-angle. Ignores X-angle and Y-angle
class Primitive
{
public:
  static std::vector<NiPoint3> Primitive::GetVertices(const espm::REFR* refr);
  static bool IsInside(const NiPoint3& point,
                       const std::vector<NiPoint3>& vertices);
};