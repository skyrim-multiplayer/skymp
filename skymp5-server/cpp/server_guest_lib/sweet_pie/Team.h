#pragma once
#include "Data.h"

namespace sweetpie {
struct Team : public Data
{
  std::set<Data> players;
  uint16_t maxTeamSize = 0;
};
}
