#pragma once
#include "GeoPolygonProc.h"
#include "NiPoint3.h"
#include "libespm/espm.h"
#include <vector>

// Supports only Z-angle. Ignores X-angle and Y-angle
class Primitive
{
public:
  static std::vector<NiPoint3> GetVertices(NiPoint3 pos, NiPoint3 rotRad,
                                           NiPoint3 boundsDiv2);
  static std::vector<NiPoint3> GetVertices(const espm::REFR* refr);
  static GeoProc::GeoPolygonProc CreateGeoPolygonProc(
    const std::vector<NiPoint3>& vertices);
  static bool IsInside(const NiPoint3& point,
                       const GeoProc::GeoPolygonProc& geoPolygonProc);
};
