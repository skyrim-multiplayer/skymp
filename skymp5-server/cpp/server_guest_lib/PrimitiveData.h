#pragma once
#include "NiPoint3.h"
#include "GeoPolygonProc.h"

struct PrimitiveData
{
  NiPoint3 boundsDiv2;
  GeoProc::GeoPolygonProc polygonProc;
};
