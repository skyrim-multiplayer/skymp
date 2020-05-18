#pragma once
#include <RE/Actor.h>
#include <RE/BSScript/ISavePatcherInterface.h>
#include <RE/BSScript/TypeTraits.h>
#include <RE/TESObjectCELL.h>
#include <RE/TESWorldSpace.h>
#include <cstdint>
#include <functional>

namespace TESModPlatform {
extern std::function<void(RE::BSScript::IVirtualMachine* vm,
                          RE::VMStackID stackId)>
  onPapyrusUpdate;

SInt32 Add(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
           RE::StaticFunctionTag*, SInt32, SInt32, SInt32, SInt32, SInt32,
           SInt32, SInt32, SInt32, SInt32, SInt32, SInt32, SInt32);

void MoveRefrToPosition(RE::BSScript::IVirtualMachine* vm,
                        RE::VMStackID stackId, RE::StaticFunctionTag*,
                        RE::TESObjectREFR* refr, RE::TESObjectCELL* cell,
                        RE::TESWorldSpace* world, float posX, float posY,
                        float posZ, float rotX, float rotY, float rotZ);

enum
{
  WEAP_DRAWN_MODE_DEFAULT = -1,
  WEAP_DRAWN_MODE_ALWAYS_FALSE = 0,
  WEAP_DRAWN_MODE_ALWAYS_TRUE = 1,

  WEAP_DRAWN_MODE_MIN = WEAP_DRAWN_MODE_DEFAULT,
  WEAP_DRAWN_MODE_MAX = WEAP_DRAWN_MODE_ALWAYS_TRUE
};

void SetWeaponDrawnMode(RE::BSScript::IVirtualMachine* vm,
                        RE::VMStackID stackId, RE::StaticFunctionTag*,
                        RE::Actor* actor, SInt32 weapDrawnMode);

int GetWeapDrawnMode(uint32_t actorId); // Threadsafe

void Update();
uint64_t GetNumPapyrusUpdates();

bool Register(RE::BSScript::IVirtualMachine* vm);
}