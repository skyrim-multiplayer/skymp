#pragma once

#include "GeoVector.h"

namespace GeoProc {

// A 3D Geometry Plane
class GeoPlane
{
public:
  // Plane Equation: A * x + B * y + C * z + D = 0

  double a;
  double b;
  double c;
  double d;

  GeoPlane(void);
  ~GeoPlane(void);

  GeoPlane(double a, double b, double c, double d)
  {
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
  }

  GeoPlane(GeoPoint p0, GeoPoint p1, GeoPoint p2)
  {

    GeoVector v = GeoVector(p0, p1);

    GeoVector u = GeoVector(p0, p2);

    GeoVector n = u * v;

    // normal vector
    this->a = n.GetX();
    this->b = n.GetY();
    this->c = n.GetZ();

    this->d = -(this->a * p0.x + this->b * p0.y + this->c * p0.z);
  }

  GeoPlane operator-()
  {
    return GeoPlane(-this->a, -this->b, -this->c, -this->d);
  }

  friend double operator*(const GeoPlane& pl, const GeoPoint& pt)
  {
    return (pt.x * pl.a + pt.y * pl.b + pt.z * pl.c + pl.d);
  }
};

}
