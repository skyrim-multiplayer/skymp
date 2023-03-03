#pragma once

#include <functional>

#include "PCH.h"

namespace TESModPlatform {
extern std::function<void(IVM* vm, StackID stackId)> onPapyrusUpdate;

int32_t Add(IVM* vm, StackID stackId, RE::StaticFunctionTag*, int32_t, int32_t,
            int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
            int32_t, int32_t, int32_t);

void MoveRefrToPosition(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
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

void SetWeaponDrawnMode(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                        RE::Actor* actor, int32_t weapDrawnMode);

int32_t GetNthVtableElement(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                            RE::TESForm* pointer, int32_t pointerOffset,
                            int32_t elementIndex);

bool IsPlayerRunningEnabled(IVM* vm, StackID stackId, RE::StaticFunctionTag*);

RE::BGSColorForm* GetSkinColor(IVM* vm, StackID stackId,
                               RE::StaticFunctionTag*, RE::TESNPC* base);

RE::TESNPC* CreateNpc(IVM* vm, StackID stackId, RE::StaticFunctionTag*);

void SetNpcSex(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
               RE::TESNPC* npc, int32_t sex);

void SetNpcRace(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                RE::TESNPC* npc, RE::TESRace* race);

void SetNpcSkinColor(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                     RE::TESNPC* npc, int32_t skinColor);

void SetNpcHairColor(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                     RE::TESNPC* npc, int32_t skinColor);

void ResizeHeadpartsArray(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                          RE::TESNPC* npc, int8_t size);

void ResizeTintsArray(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                      int32_t size);

void SetFormIdUnsafe(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                     RE::TESForm* form, uint32_t newId);

void ClearTintMasks(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                    RE::Actor* targetActor);

void PushTintMask(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                  RE::Actor* targetActor, int32_t type, uint32_t argb,
                  FixedString texturePath);

void PushWornState(IVM* vm, StackID stackId, RE::StaticFunctionTag*, bool worn,
                   bool wornLeft);

void AddItemEx(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
               RE::TESObjectREFR* containerRefr, RE::TESForm* item,
               int32_t countDelta, float health,
               RE::EnchantmentItem* enchantment, int32_t maxCharge,
               bool removeEnchantmentOnUnequip, float chargePercent,
               FixedString textDisplayData, int32_t soul,
               RE::AlchemyItem* poison, int32_t poisonCount);

void UpdateEquipment(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                     RE::Actor* containerRefr, RE::TESForm* item,
                     bool leftHand);

void ResetContainer(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                    RE::TESForm* container);

void BlockPapyrusEvents(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                        bool blocked);

// Threadsafe
void BlockMoveRefrToPosition(bool blocked);
int GetWeapDrawnMode(uint32_t actorId);
uint64_t GetNumPapyrusUpdates();
std::shared_ptr<RE::BSTArray<RE::TintMask*>> GetTintsFor(uint32_t actorId);
bool GetPapyrusEventsBlocked();

void Update();

bool Register(IVM* vm);
}
