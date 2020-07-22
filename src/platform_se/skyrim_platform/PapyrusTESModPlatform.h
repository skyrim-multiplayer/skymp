#pragma once
#include <RE/Actor.h>
#include <RE/BGSColorForm.h>
#include <RE/BSScript/ISavePatcherInterface.h>
#include <RE/BSScript/TypeTraits.h>
#include <RE/BSScript/VMArray.h>
#include <RE/TESNPC.h>
#include <RE/TESObjectCELL.h>
#include <RE/TESRace.h>
#include <RE/TESWorldSpace.h>
#include <cstdint>
#include <functional>
#include <memory>

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

SInt32 GetNthVtableElement(RE::BSScript::IVirtualMachine* vm,
                           RE::VMStackID stackId, RE::StaticFunctionTag*,
                           RE::TESForm* pointer, SInt32 pointerOffset,
                           SInt32 elementIndex);

bool IsPlayerRunningEnabled(RE::BSScript::IVirtualMachine* vm,
                            RE::VMStackID stackId, RE::StaticFunctionTag*);

RE::BGSColorForm* GetSkinColor(RE::BSScript::IVirtualMachine* vm,
                               RE::VMStackID stackId, RE::StaticFunctionTag*,
                               RE::TESNPC* base);

RE::TESNPC* CreateNpc(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
                      RE::StaticFunctionTag*);

void SetNpcSex(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
               RE::StaticFunctionTag*, RE::TESNPC* npc, SInt32 sex);

void SetNpcRace(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
                RE::StaticFunctionTag*, RE::TESNPC* npc, RE::TESRace* race);

void SetNpcSkinColor(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
                     RE::StaticFunctionTag*, RE::TESNPC* npc,
                     SInt32 skinColor);

void SetNpcHairColor(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
                     RE::StaticFunctionTag*, RE::TESNPC* npc,
                     SInt32 skinColor);

void ResizeHeadpartsArray(RE::BSScript::IVirtualMachine* vm,
                          RE::VMStackID stackId, RE::StaticFunctionTag*,
                          RE::TESNPC* npc, SInt32 size);

void ResizeTintsArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
                      RE::StaticFunctionTag*, SInt32 size);

void SetFormIdUnsafe(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
                     RE::StaticFunctionTag*, RE::TESForm* form, UInt32 newId);

void ClearTintMasks(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
                    RE::StaticFunctionTag*, RE::Actor* targetActor);

void PushTintMask(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
                  RE::StaticFunctionTag*, RE::Actor* targetActor, SInt32 type,
                  UInt32 argb, RE::BSFixedString texturePath);

// Threadsafe
void BlockMoveRefrToPosition(bool blocked);
int GetWeapDrawnMode(uint32_t actorId);
uint64_t GetNumPapyrusUpdates();
std::shared_ptr<RE::BSTArray<RE::TintMask*>> GetTintsFor(uint32_t actorId);

void Update();

bool Register(RE::BSScript::IVirtualMachine* vm);
}