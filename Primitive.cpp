#include "Primitive.h"
#include "GeoPolygon.h"
#include "GeoPolygonProc.h"
#include <cmath>

std::vector<NiPoint3> Primitive::GetVertices(NiPoint3 pos, NiPoint3 rotRad,
                                             NiPoint3 boundsDiv2)
{
  std::vector<NiPoint3> res;
  static const float pi = acosf(-1);

  for (auto zk : { -1, 1 })
    for (auto k : { 1, -1 }) {
      for (auto a : { pi / 2, -pi / 2 }) {
        auto p = pos;
        p.x += k * cos(rotRad.z) * boundsDiv2[1];
        p.x += k * cos(rotRad.z + a) * boundsDiv2[0];
        p.y += k * sin(rotRad.z) * boundsDiv2[1];
        p.y += k * sin(rotRad.z + a) * boundsDiv2[0];
        p.z += zk * boundsDiv2[2];
        res.push_back(p);
      }
    }

  return res;
}

std::vector<NiPoint3> Primitive::GetVertices(const espm::REFR* refr)
{
  espm::CompressedFieldsCache dummyCache;

  auto data = refr->GetData(dummyCache);
  NiPoint3 pos = { data.loc->pos[0], data.loc->pos[1], data.loc->pos[2] };
  NiPoint3 rotRad = { data.loc->rotRadians[0], data.loc->rotRadians[1],
                      data.loc->rotRadians[2] };
  NiPoint3 boundsDiv2 = { data.boundsDiv2[0], data.boundsDiv2[1],
                          data.boundsDiv2[2] };
  return GetVertices(pos, rotRad, boundsDiv2);
}

GeoProc::GeoPolygonProc Primitive::CreateGeoPolygonProc(
  const std::vector<NiPoint3>& vertices)
{
  static_assert(sizeof(NiPoint3) == sizeof(GeoProc::GeoPoint));

  GeoProc::GeoPolygon polygonObj = GeoProc::GeoPolygon(
    reinterpret_cast<const std::vector<GeoProc::GeoPoint>&>(vertices));
  return GeoProc::GeoPolygonProc(polygonObj);
}

bool Primitive::IsInside(const NiPoint3& point,
                         const GeoProc::GeoPolygonProc& procObj)
{
  return const_cast<GeoProc::GeoPolygonProc&>(procObj).PointInside3DPolygon(
    point.x, point.y, point.z);
}
