#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "Primitive.h"
#include "libespm/Loader.h"

extern espm::Loader l;

std::string Str(NiPoint3 p)
{
  std::stringstream ss;
  ss << "[" << static_cast<int>(p.x) << ", " << static_cast<int>(p.y) << ", "
     << static_cast<int>(p.z) << "]";
  return ss.str();
}

TEST_CASE("GetPrimitiveVertices", "[primitive][espm]")
{
  auto& br = l.GetBrowser();
  auto refr = espm::Convert<espm::REFR>(br.LookupById(0xeeb).rec);
  REQUIRE(refr);

  auto vertices = Primitive::GetVertices(refr);
  REQUIRE(Str(vertices[0]) == Str(NiPoint3(24057, -10083, -3450)));
  REQUIRE(Str(vertices[1]) == Str(NiPoint3(24267, -10300, -3450)));
  REQUIRE(Str(vertices[2]) == Str(NiPoint3(23888, -10666, -3450)));
  REQUIRE(Str(vertices[3]) == Str(NiPoint3(23678, -10450, -3450)));
  REQUIRE(Str(vertices[4]) == Str(NiPoint3(24057, -10083, -3270)));
  REQUIRE(Str(vertices[5]) == Str(NiPoint3(24267, -10300, -3270)));
  REQUIRE(Str(vertices[6]) == Str(NiPoint3(23888, -10666, -3270)));
  REQUIRE(Str(vertices[7]) == Str(NiPoint3(23678, -10450, -3270)));
}

TEST_CASE("IsInsidePrimitive", "[primitive][espm]")
{
  auto& br = l.GetBrowser();
  auto refr = espm::Convert<espm::REFR>(br.LookupById(0xeeb).rec);
  REQUIRE(refr);

  REQUIRE(Primitive::IsInside({ 24000.0000f, -10176.0000f, -3392.0000f },
                              Primitive::CreateGeoPolygonProc(
                                Primitive::GetVertices(refr))) == true);
  REQUIRE(Primitive::IsInside({ 23872.0000f, -10176.0000f, -3392.0000f },
                              Primitive::CreateGeoPolygonProc(
                                Primitive::GetVertices(refr))) == false);
}
