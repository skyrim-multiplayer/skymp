#include "Utils/Form.h"

/* we implement these first, then reorganize */
namespace Papyrus::TESModPlatform {

struct
{
  // index is formID-0xff000000
  std::vector<std::shared_ptr<RE::BSTArray<RE::TintMask*>>> actorsTints;
  std::recursive_mutex m;
} share2;

/* uint32_t TESModPlatform::Add(RE::BSScript::IVirtualMachine* vm,
                           RE::VMStackID stackId, RE::StaticFunctionTag*,
                           uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
{
  if (!papyrusUpdateAllowed)
    return 0;
  papyrusUpdateAllowed = false;

  try {
    ++numPapyrusUpdates;
    if (!onPapyrusUpdate)
      throw NullPointerException("onPapyrusUpdate");
    onPapyrusUpdate(vm, stackId);

  } catch (std::exception& e) {
    if (auto console = RE::ConsoleLog::GetSingleton())
      console->Print("Papyrus context exception: %s", e.what());
  }

  vmCallAllowed = true;

  return 0;
} */

/* void TESModPlatform::MoveRefrToPosition(
  RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
  RE::StaticFunctionTag*, RE::TESObjectREFR* refr, RE::TESObjectCELL* cell,
  RE::TESWorldSpace* world, float posX, float posY, float posZ, float rotX,
  float rotY, float rotZ)
{
  if (!refr || (!cell && !world) || moveRefrBlocked)
    return;

  NiPoint3 pos = { posX, posY, posZ }, rot = { rotX, rotY, rotZ };
  auto nullHandle = *g_invalidRefHandle;

  auto f = ::MoveRefrToPosition.operator _MoveRefrToPosition();
  f(reinterpret_cast<TESObjectREFR*>(refr), &nullHandle, cell, world, &pos,
    &rot);
} */

/* void TESModPlatform::SetWeaponDrawnMode(RE::BSScript::IVirtualMachine* vm,
                                        RE::VMStackID stackId,
                                        RE::StaticFunctionTag*,
                                        RE::Actor* actor, uint32_t
weapDrawnMode)
{
  if (!actor || weapDrawnMode < WEAP_DRAWN_MODE_MIN ||
      weapDrawnMode > WEAP_DRAWN_MODE_MAX)
    return;

  if (g_nativeCallRequirements.gameThrQ) {
    auto formId = actor->formID;
    g_nativeCallRequirements.gameThrQ->AddTask([=] {
      if (LookupFormByID(formId) != (void*)actor)
        return;

      if (!actor->IsWeaponDrawn() &&
          weapDrawnMode == WEAP_DRAWN_MODE_ALWAYS_TRUE)
        actor->DrawWeaponMagicHands(true);

      if (actor->IsWeaponDrawn() &&
          weapDrawnMode == WEAP_DRAWN_MODE_ALWAYS_FALSE)
        actor->DrawWeaponMagicHands(false);
    });
  }

  std::lock_guard l(share.m);
  share.weapDrawnMode[actor->formID] = weapDrawnMode;
} */

/* uint32_t TESModPlatform::GetNthVtableElement(RE::BSScript::IVirtualMachine*
vm, RE::VMStackID stackId, RE::StaticFunctionTag*, RE::TESForm* pointer,
uint32_t pointerOffset, uint32_t elementIndex)
{
  static auto getNthVTableElement = [](void* obj, size_t idx) {
    using VTable = size_t*;
    auto vtable = *(VTable*)obj;
    return vtable[idx];
  };

  if (pointer && elementIndex >= 0) {
    __try {
      return getNthVTableElement(reinterpret_cast<uint8_t*>(pointer) +
                                   pointerOffset,
                                 elementIndex) -
        REL::Module::BaseAddr();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
  return -1;
} */

/* RE::Module::BaseAddr() doesnt exist anymore */
uint32_t GetNthVtableElement(VM* vm, StackID stackId, RE::StaticFunctionTag*,
                             RE::TESForm* pointer, uint32_t pointerOffset,
                             uint32_t elementIndex)
{
  static auto getNthVTableElement = [](void* obj, size_t idx) {
    using VTable = size_t*;
    auto vtable = *(VTable*)obj;
    return vtable[idx];
  };

  if (pointer && elementIndex >= 0) {
    __try {
      return getNthVTableElement(reinterpret_cast<uint8_t*>(pointer) +
                                   pointerOffset,
                                 elementIndex) -
        REL::Module::get().base(); // ???
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
  return -1;
}

/* bool TESModPlatform::IsPlayerRunningEnabled(RE::BSScript::IVirtualMachine*
vm, RE::VMStackID stackId, RE::StaticFunctionTag*)
{
  if (auto controls = RE::PlayerControls::GetSingleton())
    return controls->data.running;
  return false;
} */

/* no changes */
bool IsPlayerRunningEnabled(VM* vm, StackID stackId, RE::StaticFunctionTag*)
{
  if (auto controls = RE::PlayerControls::GetSingleton())
    return controls->data.running;
  return false;
}

/* RE::BGSColorForm* TESModPlatform::GetSkinColor(
  RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
  RE::StaticFunctionTag*, RE::TESNPC* base)
{
  auto factory = IFormFactory::GetFactoryForType(BGSColorForm::kTypeID);
  if (!factory)
    return nullptr;
  auto col = (RE::BGSColorForm*)factory->Create();
  if (!col)
    return nullptr;
  col->color = base->bodyTintColor;
  return col;
} */

/* doesn't use SKSE now */
/* streamlined function name with the rest of them */
RE::BGSColorForm* TESModPlatform::GetNPCSkinColor(VM* vm, StackID stackId,
                                                  RE::StaticFunctionTag*,
                                                  RE::TESNPC* npc)
{
  auto cf = Utils::CreateForm<RE::BGSColorForm>(RE::FormType::ColorForm);
  if (!cf)
    return nullptr;
  cf->color = npc->bodyTintColor;
  return cf;
}

/* since this is papyrus RE::BGSColorForm* colorForm looks more logical... or
 * not? */
void SetNPCSkinColor(VM* vm, StackID stackId, RE::StaticFunctionTag*,
                     RE::TESNPC* npc, RE::BGSColorForm* colorForm)
{
  npc->bodyTintColor = colorForm->color;
}

/* RE::TESNPC* TESModPlatform::CreateNpc(RE::BSScript::IVirtualMachine* vm,
                                      RE::VMStackID stackId,
                                      RE::StaticFunctionTag*)
{
  auto factory = IFormFactory::GetFactoryForType(TESNPC::kTypeID);
  if (!factory)
    return nullptr;

  auto npc = (RE::TESNPC*)factory->Create();
  if (!npc)
    return nullptr;

  enum
  {
    AADeleteWhenDoneTestJeremyRegular = 0x0010D13E
  };
  const auto srcNpc =
    (TESNPC*)LookupFormByID(AADeleteWhenDoneTestJeremyRegular);
  assert(srcNpc);
  assert(srcNpc->formType == kFormType_NPC);
  if (!srcNpc || srcNpc->formType != kFormType_NPC)
    return nullptr;
  auto backup = npc->formID;
  memcpy(npc, srcNpc, sizeof TESNPC);
  npc->formID = backup;

  auto npc_ = (TESNPC*)npc;
  npc_->container.entries = nullptr;
  npc_->container.numEntries = 0;
  npc_->faction = nullptr;
  npc_->nextTemplate = nullptr;
  npc_->actorData.flags |= (1 << 7);  // pcLevelMult
  npc_->actorData.flags |= (1 << 5);  // unique
  npc_->actorData.flags |= (1 << 14); // simpleActor, Disables face animations

  // Clear AI Packages to prevent idle animations with Furniture
  enum
  {
    DoNothing = 0x654e2,
    DefaultMoveToCustom02IgnoreCombat = 0x6af62
  };
  auto doNothing = (TESPackage*)LookupFormByID(DoNothing);
  auto flagsSource = (TESPackage*)LookupFormByID(
    DefaultMoveToCustom02IgnoreCombat); // ignore combat && no
                                        // combat alert
  doNothing->packageFlags = flagsSource->packageFlags;
  npc_->aiForm.unk18.unk0 = doNothing;
  npc_->aiForm.unk18.next = nullptr;

  auto sourceMorph = npc_->faceMorph;
  npc_->faceMorph =
    (TESNPC::FaceMorphs*)Heap_Allocate(sizeof(TESNPC::FaceMorphs));
  if (!npc_->faceMorph)
    return nullptr;
  *npc_->faceMorph = *sourceMorph;

  return (RE::TESNPC*)npc;
} */

/* void TESModPlatform::SetNpcSex(RE::BSScript::IVirtualMachine* vm,
                               RE::VMStackID stackId, RE::StaticFunctionTag*,
                               RE::TESNPC* npc, uint32_t sex)
{
  if (npc) {
    if (sex == 1)
      npc->actorData.actorBaseFlags |= RE::ACTOR_BASE_DATA::Flag::kFemale;
    else
      npc->actorData.actorBaseFlags &= ~RE::ACTOR_BASE_DATA::Flag::kFemale;
  }
} */

/* no idea, skse enumeration must have changed */
void SetNPCSex(VM* vm, StackID stackId, RE::StaticFunctionTag*,
               RE::TESNPC* npc, RE::SEX sex)
{
  if (npc) {
    if (sex == RE::SEX::kFemale) // ???
      npc->actorData.actorBaseFlags |= RE::ACTOR_BASE_DATA::Flag::kFemale;
    else
      npc->actorData.actorBaseFlags &=
        ~RE::ACTOR_BASE_DATA::Flag::kFemale; // error here
  }
}

/* void TESModPlatform::SetNpcRace(RE::BSScript::IVirtualMachine* vm,
                                RE::VMStackID stackId, RE::StaticFunctionTag*,
                                RE::TESNPC* npc, RE::TESRace* race)
{
  if (npc && race)
    npc->race = race;
} */

/* no changes */
void SetNpcRace(VM* vm, StackID stackId, RE::StaticFunctionTag*,
                RE::TESNPC* npc, RE::TESRace* race)
{
  if (npc && race)
    npc->race = race;
}

/* void TESModPlatform::ResizeHeadpartsArray(RE::BSScript::IVirtualMachine* vm,
                                          RE::VMStackID stackId,
                                          RE::StaticFunctionTag*,
                                          RE::TESNPC* npc, uint32_t newSize)
{
  if (!npc)
    return;
  if (newSize <= 0) {
    npc->headParts = nullptr;
    npc->numHeadParts = 0;
  } else {
    npc->headParts = new RE::BGSHeadPart*[newSize];
    npc->numHeadParts = (uint8_t)newSize;
    for (SInt8 i = 0; i < npc->numHeadParts; ++i)
      npc->headParts[i] = nullptr;
  }
} */

/* only changed int type, gotta check later */
void ResizeHeadpartsArray(VM* vm, StackID stackId, RE::StaticFunctionTag*,
                          RE::TESNPC* npc, uint32_t newSize)
{
  if (!npc)
    return;
  if (newSize <= 0) {
    npc->headParts = nullptr;
    npc->numHeadParts = 0;
  } else {
    npc->headParts = new RE::BGSHeadPart*[newSize];
    npc->numHeadParts = (uint8_t)newSize;
    for (uint8_t i = 0; i < npc->numHeadParts; ++i)
      npc->headParts[i] = nullptr;
  }
}

/* void TESModPlatform::ResizeTintsArray(RE::BSScript::IVirtualMachine* vm,
                                      RE::VMStackID stackId,
                                      RE::StaticFunctionTag*, uint32_t newSize)
{
  PlayerCharacter* pc = *g_thePlayer;
  RE::PlayerCharacter* rePc = (RE::PlayerCharacter*)pc;
  if (!rePc)
    return;
  if (newSize < 0 || newSize > 1024)
    return;
  auto prevSize = rePc->tintMasks.size();
  rePc->tintMasks.resize(newSize);
  for (size_t i = prevSize; i < rePc->tintMasks.size(); ++i) {
    rePc->tintMasks[i] = (RE::TintMask*)new TintMask;
  }
} */

/* partly removed skse dependency */
void ResizeTintsArray(VM* vm, StackID stackId, RE::StaticFunctionTag*,
                      uint32_t newSize)
{
  auto pc = RE::PlayerCharacter::GetSingleton();
  if (!pc)
    return;

  auto prevSize = pc->tintMasks.size();

  if (newSize < 0 || newSize > 1024 || newSize == prevSize)
    return;

  pc->tintMasks.resize(newSize);
  for (uint32_t i = prevSize; i < pc->tintMasks.size(); ++i) {
    pc->tintMasks[i] =
      (RE::TintMask*)new TintMask; // this uses TintMask from skse
  }
}

/* void TESModPlatform::SetFormIdUnsafe(RE::BSScript::IVirtualMachine* vm,
                                     RE::VMStackID stackId,
                                     RE::StaticFunctionTag*, RE::TESForm* form,
                                     UInt32 newId)
{
  if (form)
    form->formID = newId;
} */

/* no changes */
void SetFormIdUnsafe(VM* vm, StackID stackId, RE::StaticFunctionTag*,
                     RE::TESForm* form, uint32_t newId)
{
  if (form)
    form->formID = newId;
}

/* void TESModPlatform::ClearTintMasks(RE::BSScript::IVirtualMachine* vm,
                                    RE::VMStackID stackId,
                                    RE::StaticFunctionTag*,
                                    RE::Actor* targetActor)
{
  if (!targetActor) {
    auto pc = RE::PlayerCharacter::GetSingleton();
    return pc->tintMasks.clear();
  }

  if (targetActor->formID < 0xff000000)
    return;
  size_t i = targetActor->formID - 0xff000000;

  std::lock_guard l(share2.m);
  if (share2.actorsTints.size() > i)
    share2.actorsTints[i].reset();
} */

/* no changes */
void ClearTintMasks(VM* vm, StackID stackId, RE::StaticFunctionTag*,
                    RE::Actor* targetActor)
{
  if (!targetActor) {
    auto pc = RE::PlayerCharacter::GetSingleton();
    return pc->tintMasks.clear();
  }

  if (targetActor->formID < 0xff000000)
    return;

  uint32_t i = targetActor->formID - 0xff000000;

  std::lock_guard l(share2.m);
  if (share2.actorsTints.size() > i)
    share2.actorsTints[i].reset();
}

/* void TESModPlatform::PushTintMask(RE::BSScript::IVirtualMachine* vm,
                                  RE::VMStackID stackId,
                                  RE::StaticFunctionTag*,
                                  RE::Actor* targetActor, uint32_t type,
                                  UInt32 argb, RE::BSFixedString texturePath)
{
  auto newTm = (TintMask*)Heap_Allocate(sizeof TintMask);
  if (!newTm)
    return;

  ARGBColor color(argb);
  float alpha = color.GetAlpha() / 255.f;
  if (alpha > 1.0)
    alpha = 1.f;
  if (alpha < 0.0)
    alpha = 0.f;
  newTm->color.alpha = color.GetAlpha();
  newTm->alpha = alpha;
  newTm->color.red = color.GetRed();
  newTm->color.green = color.GetGreen();
  newTm->color.blue = color.GetBlue();

  newTm->texture = (TESTexture*)Heap_Allocate(sizeof TESTexture);
  if (!newTm->texture)
    return;
  newTm->texture->str = *(BSFixedString*)&texturePath;

  newTm->tintType = type;

  if (targetActor == nullptr) {
    auto targetArray = &RE::PlayerCharacter::GetSingleton()->tintMasks;
    auto n = targetArray->size();
    targetArray->resize(1 + n);
    if (targetArray->size() == 1 + n) {
      targetArray->back() = (RE::TintMask*)newTm;
    }
    return;
  }

  if (targetActor->formID < 0xff000000)
    return;

  size_t i = targetActor->formID - 0xff000000;
  std::lock_guard l(share2.m);
  share2.actorsTints.resize(std::max(i + 1, share2.actorsTints.size()));

  std::shared_ptr<RE::BSTArray<RE::TintMask*>> tints(
    new RE::BSTArray<RE::TintMask*>);
  if (share2.actorsTints[i]) {
    *tints = Clone(*share2.actorsTints[i]);
  }
  auto n = tints->size();
  tints->resize(1 + n);
  if (tints->size() == 1 + n) {
    tints->back() = (RE::TintMask*)newTm;
  }

  share2.actorsTints[i] = tints;
} */

void TESModPlatform::PushTintMask(VM* vm, StackID stackId,
                                  RE::StaticFunctionTag*,
                                  RE::Actor* targetActor, uint32_t type,
                                  uint32_t argb, RE::BSFixedString texturePath)
{
  auto newTm = (TintMask*)Heap_Allocate(sizeof TintMask);
  if (!newTm)
    return;

  ARGBColor color(argb);
  float alpha = color.GetAlpha() / 255.f;
  if (alpha > 1.0)
    alpha = 1.f;
  if (alpha < 0.0)
    alpha = 0.f;
  newTm->color.alpha = color.GetAlpha();
  newTm->alpha = alpha;
  newTm->color.red = color.GetRed();
  newTm->color.green = color.GetGreen();
  newTm->color.blue = color.GetBlue();

  newTm->texture = (TESTexture*)Heap_Allocate(sizeof TESTexture);
  if (!newTm->texture)
    return;
  newTm->texture->str = *(BSFixedString*)&texturePath;

  newTm->tintType = type;

  if (targetActor == nullptr) {
    auto targetArray = &RE::PlayerCharacter::GetSingleton()->tintMasks;
    auto n = targetArray->size();
    targetArray->resize(1 + n);
    if (targetArray->size() == 1 + n) {
      targetArray->back() = (RE::TintMask*)newTm;
    }
    return;
  }

  if (targetActor->formID < 0xff000000)
    return;

  size_t i = targetActor->formID - 0xff000000;
  std::lock_guard l(share2.m);
  share2.actorsTints.resize(std::max(i + 1, share2.actorsTints.size()));

  std::shared_ptr<RE::BSTArray<RE::TintMask*>> tints(
    new RE::BSTArray<RE::TintMask*>);
  if (share2.actorsTints[i]) {
    *tints = Clone(*share2.actorsTints[i]);
  }
  auto n = tints->size();
  tints->resize(1 + n);
  if (tints->size() == 1 + n) {
    tints->back() = (RE::TintMask*)newTm;
  }

  share2.actorsTints[i] = tints;
}

/* void TESModPlatform::PushWornState(RE::BSScript::IVirtualMachine* vm,
                                   RE::VMStackID stackId,
                                   RE::StaticFunctionTag*, bool worn,
                                   bool wornLeft)
{
  g_worn = worn;
  g_wornLeft = wornLeft;
} */

/* void TESModPlatform::AddItemEx(
  RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
  RE::StaticFunctionTag*, RE::TESObjectREFR* containerRefr, RE::TESForm* item,
  uint32_t countDelta, float health, RE::EnchantmentItem* enchantment,
  uint32_t maxCharge, bool removeEnchantmentOnUnequip, float chargePercent,
  RE::BSFixedString textDisplayData, uint32_t soul, RE::AlchemyItem* poison,
  uint32_t poisonCount)
{
  auto ui = RE::UI::GetSingleton();
  if (!containerRefr || !item || !ui || ui->GameIsPaused())
    return;

  const auto refrId = containerRefr->GetFormID();

  auto boundObject = reinterpret_cast<RE::TESBoundObject*>(
    DYNAMIC_CAST(reinterpret_cast<TESForm*>(item), TESForm, TESBoundObject));
  if (!boundObject)
    return;

  auto tuple = std::make_tuple(
    containerRefr->formID, item, health, enchantment, maxCharge,
    removeEnchantmentOnUnequip, chargePercent,
    (std::string)textDisplayData.data(), soul, poison, poisonCount);

  using Tuple = decltype(tuple);

  thread_local std::map<Tuple, RE::ExtraDataList*> g_lastEquippedExtraList[2];

  RE::ExtraDataList* extraList = nullptr;

  const bool isShieldLike =
    (item->formType == RE::FormType::Armor &&
     reinterpret_cast<RE::TESObjectARMO*>(item)->IsShield());

  const bool isTorch = item->formType == RE::FormType::Light;

  const bool isClothes =
    (item->formType == RE::FormType::Armor && !isShieldLike) ||
    item->formType == RE::FormType::Light;

  // Our extra-less items support is disgusting! EquipItem crashes when we try
  // an iron sword. This hack saves our slav lives
  if (item->formType != RE::FormType::Ammo && health <= 1)
    health = 1.01f;

  if (health > 1 || enchantment || chargePercent > 0 ||
      strlen(textDisplayData.data()) > 0 || (soul > 0 && soul <= 5) ||
      poison || g_worn || g_wornLeft) {
    extraList = CreateExtraDataList();

    auto extraList_ = reinterpret_cast<BaseExtraList*>(extraList);

    if (g_worn) {
      if (isClothes) {
        auto extra =
          reinterpret_cast<BSExtraData*>(new MyExtra<kExtraData_Worn>);
        extraList_->Add(kExtraData_Worn, extra);
      }
    }

    if (g_wornLeft) {
      if (isClothes) {
        auto extra =
          reinterpret_cast<BSExtraData*>(new MyExtra<kExtraData_WornLeft>);
        extraList_->Add(kExtraData_WornLeft, extra);
      }
    }

    if (health > 1)
      extraList_->Add(kExtraData_Health,
                      (BSExtraData*)new RE::ExtraHealth(health));
    if (enchantment)
      extraList_->Add(kExtraData_Enchantment,
                      (BSExtraData*)new RE::ExtraEnchantment(
                        enchantment, maxCharge, removeEnchantmentOnUnequip));
    if (chargePercent > 0) {
      auto extraCharge = new RE::ExtraCharge;
      extraCharge->charge = chargePercent;
      extraList_->Add(kExtraData_Charge, (BSExtraData*)extraCharge);
    }
    if (strlen(textDisplayData.data()) > 0)
      extraList_->Add(
        kExtraData_TextDisplayData,
        (BSExtraData*)new RE::ExtraTextDisplayData(textDisplayData.data()));
    if (soul > 0 && soul <= 5)
      extraList_->Add(
        kExtraData_Soul,
        (BSExtraData*)new RE::ExtraSoul(static_cast<RE::SOUL_LEVEL>(soul)));
    if (poison) {
      extraList_->Add(kExtraData_Poison,
                      (BSExtraData*)new RE::ExtraPoison(poison, poisonCount));
    }
  }

  g_nativeCallRequirements.gameThrQ->AddTask([=] {
    if (containerRefr != (void*)LookupFormByID(refrId))
      return;

    auto optExtraList =
      item->formType == RE::FormType::Ammo ? nullptr : extraList;

    if (countDelta > 0) {
      containerRefr->AddObjectToContainer(boundObject, optExtraList,
                                          countDelta, nullptr);
    } else if (countDelta < 0) {
      containerRefr->RemoveItem(boundObject, -countDelta,
                                RE::ITEM_REMOVE_REASON::kRemove, optExtraList,
                                nullptr);
    }
  });

  const bool needEquipWeap =
    (g_worn || g_wornLeft) && item->formType == RE::FormType::Weapon;

  const bool needEquipShieldLike = (g_worn || g_wornLeft) && isShieldLike;

  const bool needEquipAmmo =
    (g_worn || g_wornLeft) && item->formType == RE::FormType::Ammo;

  if (needEquipWeap || needEquipShieldLike || needEquipAmmo) {
    auto s = RE::ActorEquipManager::GetSingleton();
    if (containerRefr->formType == RE::FormType::ActorCharacter) {

      enum EquipSlot
      {
        BothHands = 0x13f45,
        LeftHand = 0x13f43,
        RightHand = 0x13f42
      };
      static const auto g_bothHandsSlot = LookupFormByID(BothHands);

      RE::Actor* actor = reinterpret_cast<RE::Actor*>(containerRefr);
      if (s) {
        auto slot = reinterpret_cast<RE::BGSEquipSlot*>(GetRightHandSlot());

        if (g_wornLeft && !needEquipShieldLike) // wornLeft + shield = deadlock
          slot = reinterpret_cast<RE::BGSEquipSlot*>(GetLeftHandSlot());

        if (item->formType == RE::FormType::Ammo) {
          extraList = nullptr;
          slot = nullptr;
        }

        if (countDelta > 0) {
          g_lastEquippedExtraList[g_worn ? false : true][tuple] = extraList;
          g_nativeCallRequirements.gameThrQ->AddTask([=] {
            if (actor != (void*)LookupFormByID(refrId))
              return;
            s->EquipObject(actor, boundObject, extraList, 1, slot);
          });
        } else if (countDelta < 0)
          g_nativeCallRequirements.gameThrQ->AddTask([=] {
            if (actor != (void*)LookupFormByID(refrId))
              return;
            s->UnequipObject(actor, boundObject, extraList, 1, slot);
          });
      }
    }
  }

  g_worn = false;
  g_wornLeft = false;
} */

/* void TESModPlatform::UpdateEquipment(RE::BSScript::IVirtualMachine* vm,
                                     RE::VMStackID stackId,
                                     RE::StaticFunctionTag*,
                                     RE::Actor* containerRefr,
                                     RE::TESForm* item, bool leftHand)
{

  auto ac = ((Actor*)containerRefr);

  if (!ac || !ac->processManager)
    return;

  auto& ref =
    ac->processManager
      ->equippedObject[leftHand ? ActorProcessManager::kEquippedHand_Left
                                : ActorProcessManager::kEquippedHand_Right];

  const auto backup = ref;
  ref = (TESForm*)item;
} */

/* void TESModPlatform::ResetContainer(RE::BSScript::IVirtualMachine* vm,
                                    RE::VMStackID stackId,
                                    RE::StaticFunctionTag*,
                                    RE::TESForm* container)
{
  if (!container)
    return;
  TESContainer* pContainer =
    DYNAMIC_CAST(reinterpret_cast<TESForm*>(container), TESForm, TESContainer);
  if (!pContainer)
    return;
  pContainer->numEntries = 0;
  pContainer->entries = nullptr;
} */

/* void TESModPlatform::BlockPapyrusEvents(RE::BSScript::IVirtualMachine* vm,
                                        RE::VMStackID stackId,
                                        RE::StaticFunctionTag*, bool blocked)
{
  papyrusEventsBlocked = blocked;
} */

/* int TESModPlatform::GetWeapDrawnMode(uint32_t actorId)
{
  std::lock_guard l(share.m);
  auto it = share.weapDrawnMode.find(actorId);
  return it == share.weapDrawnMode.end() ? WEAP_DRAWN_MODE_DEFAULT
                                         : it->second;
} */

void Bind(VM& a_vm)
{
  /* register everything */
}

}
