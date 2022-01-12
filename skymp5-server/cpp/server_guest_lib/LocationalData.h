#pragma once
#include "FormDesc.h"
#include "NiPoint3.h"
#include <tuple>

struct LocationalData
{
  NiPoint3 pos, rot;
  FormDesc cellOrWorldDesc;

  friend bool operator==(const LocationalData& r, const LocationalData& l)
  {
    return r.pos == l.pos && r.rot == l.rot &&
      r.cellOrWorldDesc == l.cellOrWorldDesc;
  }

  friend bool operator!=(const LocationalData& r, const LocationalData& l)
  {
    return !(r == l);
  }

  friend bool operator<(const LocationalData& r, const LocationalData& l)
  {
    return std::make_tuple(r.rot[0], r.rot[1], r.rot[2], r.pos[0], r.pos[1],
                           r.pos[2], r.cellOrWorldDesc) <
      std::make_tuple(l.rot[0], l.rot[1], l.rot[2], l.pos[0], l.pos[1],
                      l.pos[2], l.cellOrWorldDesc);
  }
};
