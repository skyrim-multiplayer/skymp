#pragma once
#include <RE\BSScript\ISavePatcherInterface.h>
#include <RE\BSScript\TypeTraits.h>
#include <cstdint>
#include <functional>

namespace TESModPlatform {
extern std::function<void(RE::BSScript::IVirtualMachine* vm,
                          RE::VMStackID stackId)>
  onPapyrusUpdate;

SInt32 Add(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
           RE::StaticFunctionTag*, SInt32, SInt32, SInt32, SInt32, SInt32,
           SInt32, SInt32, SInt32, SInt32, SInt32, SInt32, SInt32);

void Update();

uint64_t GetNumPapyrusUpdates();

bool Register(RE::BSScript::IVirtualMachine* vm);
}