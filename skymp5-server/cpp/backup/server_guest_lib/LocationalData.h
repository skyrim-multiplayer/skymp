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
    return std::make_tuple(r.pos, r.rot, r.cellOrWorldDesc) <
      std::make_tuple(l.pos, l.rot, l.cellOrWorldDesc);
  }
};
