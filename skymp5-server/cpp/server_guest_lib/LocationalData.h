#pragma once
#include "FormDesc.h"
#include "NiPoint3.h"
#include <tuple>

struct LocationalData
{
  NiPoint3 pos, rot;
  FormDesc cellOrWorldDesc;
};
