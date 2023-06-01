#include "Override.h"

std::atomic<bool> Override::g_overriden = false;

Override::Override()
{
  g_overriden = true;
}

Override::~Override()
{
  g_overriden = false;
}

bool Override::IsOverriden()
{
  return g_overriden;
}
