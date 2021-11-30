#pragma once

#include "Extensions/TESModPlatform.h"

namespace Papyrus {

bool Bind(VM* a_vm)
{
  if (!a_vm) {
    logger::critical("no VM @ papyrus handler"sv);
    return false;
  }

  /* TESModPlatform::Bind(*a_vm); */

  return true;
}

}
