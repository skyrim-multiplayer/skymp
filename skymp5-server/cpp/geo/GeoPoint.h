#pragma once

namespace GeoProc {
// A 3D Geometry Point
class GeoPoint
{

public:
  float x = 0.f;
  float y = 0.f;
  float z = 0.f;

  GeoPoint(void);
  ~GeoPoint(void);

  GeoPoint(float x_, float y_, float z_)
    : x(x_)
    , y(y_)
    , z(z_)
  {
  }

  friend GeoPoint operator+(const GeoPoint& p0, const GeoPoint& p1)
  {
    return GeoPoint(p0.x + p1.x, p0.y + p1.y, p0.z + p1.z);
  }
};

}
