#pragma once
#include "Script.h"

#pragma pack(push, 1)

namespace espm {

struct ScriptData
{
  int16_t version = 0;   // [2..5]
  int16_t objFormat = 0; // [1..2]
  std::vector<Script> scripts;
};

}

#pragma pack(pop)
