#pragma once
#include "GeoFace.h"
#include "GeoPlane.h"
#include "GeoPoint.h"
#include "GeoPolygon.h"
#include "GeoVector.h"
#include "Utility.h"
#include <vector>

namespace GeoProc {
// 3D Polygon Process
class GeoPolygonProc
{
  // Polygon
  GeoPolygon _polygon;

  // Polygon Boundary
  double _x0, _x1, _y0, _y1, _z0, _z1;

  // Polygon faces
  std::vector<GeoFace> _Faces;

  // Polygon face planes
  std::vector<GeoPlane> _FacePlanes;

  // Number of faces
  int _NumberOfFaces;

  // Maximum point to face plane distance error, point is considered in the
  // face plane if its distance is less than this error
  double _MaxDisError;
  void Set3DPolygonBoundary();
  void Set3DPolygonUnitError();
  void SetConvex3DFaces();

public:
  ~GeoPolygonProc(void) {}

  GeoPolygonProc(GeoPolygon polygon)
  {
    _polygon = polygon;
    Set3DPolygonBoundary();
    Set3DPolygonUnitError();
    SetConvex3DFaces();
  }

  GeoPolygon GetPolygon() { return _polygon; }

  double GetX0() { return _x0; }
  double GetX1() { return _x1; }
  double GetY0() { return _y0; }
  double GetY1() { return _y1; }
  double GetZ0() { return _z0; }
  double GetZ1() { return _z1; }

  std::vector<GeoFace> GetFaces() { return _Faces; }
  std::vector<GeoPlane> GetFacePlanes() { return _FacePlanes; }
  int GetNumberOfFaces() { return _NumberOfFaces; }
  double GetMaxDisError() { return _MaxDisError; }
  void SetMaxDisError(double value) { _MaxDisError = value; }
  bool PointInside3DPolygon(double x, double y, double z);
};

}
