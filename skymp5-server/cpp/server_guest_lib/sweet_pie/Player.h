#pragma once
#include "Data.h"

namespace sweetpie {
struct Player : public Data
{
  Data* team = nullptr;
};
}
