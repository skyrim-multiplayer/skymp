#include "PapyrusTESModPlatform.h"
#include "CallNativeApi.h"
#include "NullPointerException.h"
#include <RE/AIProcess.h>
#include <RE/ActorEquipManager.h>
#include <RE/AlchemyItem.h>
#include <RE/BGSEquipSlot.h>
#include <RE/BSScript/IFunctionArguments.h>
#include <RE/BSScript/IStackCallbackFunctor.h>
#include <RE/BSScript/NativeFunction.h>
#include <RE/ConsoleLog.h>
#include <RE/EnchantmentItem.h>
#include <RE/ExtraCharge.h>
#include <RE/ExtraEnchantment.h>
#include <RE/ExtraHealth.h>
#include <RE/ExtraPoison.h>
#include <RE/ExtraShouldWear.h>
#include <RE/ExtraSoul.h>
#include <RE/ExtraTextDisplayData.h>
#include <RE/ExtraWorn.h>
#include <RE/ExtraWornLeft.h>
#include <RE/MiddleHighProcessData.h>
#include <RE/Offsets.h>
#include <RE/PlayerCharacter.h>
#include <RE/PlayerControls.h>
#include <RE/ScriptEventSourceHolder.h>
#include <RE/SkyrimVM.h>
#include <RE/TESForm.h>
#include <RE/TESNPC.h>
#include <RE/TESObjectARMO.h>
#include <RE/TESObjectWEAP.h>
#include <RE/UI.h>
#include <atomic>
#include <map>
#include <mutex>
#include <re/BGSEquipSlot.h>
#include <re/Offsets_RTTI.h>
#include <skse64/Colors.h>
#include <skse64/GameData.h>
#include <skse64/GameExtraData.h>
#include <skse64/GameForms.h> // IFormFactory::GetFactoryForType
#include <skse64/GameRTTI.h>
#include <skse64/GameReferences.h>
#include <skse64/NiNodes.h>
#include <skse64/PapyrusGame.h>
#include <unordered_map>
#include <unordered_set>

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace TESModPlatform {
bool papyrusUpdateAllowed = false;
bool vmCallAllowed = true;
std::atomic<bool> moveRefrBlocked = false;
std::function<void(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId)>
  onPapyrusUpdate = nullptr;
std::atomic<uint64_t> numPapyrusUpdates = 0;
struct
{
  std::unordered_map<uint32_t, int> weapDrawnMode;
  std::recursive_mutex m;
} share;
std::atomic<bool> papyrusEventsBlocked;

struct
{
  // index is formID-0xff000000
  std::vector<std::shared_ptr<RE::BSTArray<RE::TintMask*>>> actorsTints;
  std::recursive_mutex m;
} share2;

namespace {
RE::BSTArray<RE::TintMask*> Clone(const RE::BSTArray<RE::TintMask*>& original)
{
  RE::BSTArray<RE::TintMask*> res;
  for (auto tint : original) {
    res.push_back(nullptr);
    res.back() = (RE::TintMask*)Heap_Allocate(sizeof(TintMask));
    memcpy(res.back(), tint, sizeof(TintMask));
  }
  return res;
}
}

class FunctionArguments : public RE::BSScript::IFunctionArguments
{
public:
  bool operator()(
    RE::BSScrapArray<RE::BSScript::Variable>& a_dst) const override
  {
    a_dst.resize(12);
    for (int i = 0; i < 12; i++) {
      a_dst[i].SetSInt(100 + i);
    }
    return true;
  }
};

class StackCallbackFunctor : public RE::BSScript::IStackCallbackFunctor
{
public:
  void operator()(RE::BSScript::Variable a_result) override {}
  bool CanSave() const override { return false; }
  void SetObject(
    const RE::BSTSmartPointer<RE::BSScript::Object>& a_object) override{};
};

// This class has been added as an issue 52 workaround
class LoadGameEvent : public RE::BSTEventSink<RE::TESLoadGameEvent>
{
public:
  LoadGameEvent()
  {
    auto holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (!holder)
      throw NullPointerException("holder");

    holder->AddEventSink(this);
  }

private:
  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESLoadGameEvent* event_,
    RE::BSTEventSource<RE::TESLoadGameEvent>* eventSource) override
  {
    vmCallAllowed = true;
    return RE::BSEventNotifyControl::kContinue;
  }
};
}

SInt32 TESModPlatform::Add(RE::BSScript::IVirtualMachine* vm,
                           RE::VMStackID stackId, RE::StaticFunctionTag*,
                           SInt32, SInt32, SInt32, SInt32, SInt32, SInt32,
                           SInt32, SInt32, SInt32, SInt32, SInt32, SInt32)
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
}

void TESModPlatform::MoveRefrToPosition(
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
}

void TESModPlatform::BlockMoveRefrToPosition(bool blocked)
{
  moveRefrBlocked = blocked;
}

void TESModPlatform::SetWeaponDrawnMode(RE::BSScript::IVirtualMachine* vm,
                                        RE::VMStackID stackId,
                                        RE::StaticFunctionTag*,
                                        RE::Actor* actor, SInt32 weapDrawnMode)
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
}

SInt32 TESModPlatform::GetNthVtableElement(RE::BSScript::IVirtualMachine* vm,
                                           RE::VMStackID stackId,
                                           RE::StaticFunctionTag*,
                                           RE::TESForm* pointer,
                                           SInt32 pointerOffset,
                                           SInt32 elementIndex)
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
}

bool TESModPlatform::IsPlayerRunningEnabled(RE::BSScript::IVirtualMachine* vm,
                                            RE::VMStackID stackId,
                                            RE::StaticFunctionTag*)
{
  if (auto controls = RE::PlayerControls::GetSingleton())
    return controls->data.running;
  return false;
}

RE::BGSColorForm* TESModPlatform::GetSkinColor(
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
}

RE::TESNPC* TESModPlatform::CreateNpc(RE::BSScript::IVirtualMachine* vm,
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
}

void TESModPlatform::SetNpcSex(RE::BSScript::IVirtualMachine* vm,
                               RE::VMStackID stackId, RE::StaticFunctionTag*,
                               RE::TESNPC* npc, SInt32 sex)
{
  if (npc) {
    if (sex == 1)
      npc->actorData.actorBaseFlags |= RE::ACTOR_BASE_DATA::Flag::kFemale;
    else
      npc->actorData.actorBaseFlags &= ~RE::ACTOR_BASE_DATA::Flag::kFemale;
  }
}

void TESModPlatform::SetNpcRace(RE::BSScript::IVirtualMachine* vm,
                                RE::VMStackID stackId, RE::StaticFunctionTag*,
                                RE::TESNPC* npc, RE::TESRace* race)
{
  if (npc && race)
    npc->race = race;
}

void TESModPlatform::SetNpcSkinColor(RE::BSScript::IVirtualMachine* vm,
                                     RE::VMStackID stackId,
                                     RE::StaticFunctionTag*, RE::TESNPC* npc,
                                     SInt32 color)
{
  if (!npc)
    return;
  npc->bodyTintColor.red = COLOR_RED(color);
  npc->bodyTintColor.green = COLOR_GREEN(color);
  npc->bodyTintColor.blue = COLOR_BLUE(color);
}

void TESModPlatform::SetNpcHairColor(RE::BSScript::IVirtualMachine* vm,
                                     RE::VMStackID stackId,
                                     RE::StaticFunctionTag*, RE::TESNPC* npc,
                                     SInt32 color)
{
  if (!npc)
    return;
  if (!npc->headRelatedData) {
    npc->headRelatedData = (RE::TESNPC::HeadRelatedData*)Heap_Allocate(
      sizeof RE::TESNPC::HeadRelatedData);
    if (!npc->headRelatedData)
      return;
    npc->headRelatedData->faceDetails = nullptr;
  }

  auto factory = IFormFactory::GetFactoryForType(BGSColorForm::kTypeID);
  if (!factory)
    return;

  npc->headRelatedData->hairColor = (RE::BGSColorForm*)factory->Create();
  if (!npc->headRelatedData->hairColor)
    return;

  auto& c = npc->headRelatedData->hairColor->color;
  c.red = COLOR_RED(color);
  c.green = COLOR_GREEN(color);
  c.blue = COLOR_BLUE(color);
}

void TESModPlatform::ResizeHeadpartsArray(RE::BSScript::IVirtualMachine* vm,
                                          RE::VMStackID stackId,
                                          RE::StaticFunctionTag*,
                                          RE::TESNPC* npc, SInt32 newSize)
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
}

void TESModPlatform::ResizeTintsArray(RE::BSScript::IVirtualMachine* vm,
                                      RE::VMStackID stackId,
                                      RE::StaticFunctionTag*, SInt32 newSize)
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
}

void TESModPlatform::SetFormIdUnsafe(RE::BSScript::IVirtualMachine* vm,
                                     RE::VMStackID stackId,
                                     RE::StaticFunctionTag*, RE::TESForm* form,
                                     UInt32 newId)
{
  if (form)
    form->formID = newId;
}

void TESModPlatform::ClearTintMasks(RE::BSScript::IVirtualMachine* vm,
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
}

void TESModPlatform::PushTintMask(RE::BSScript::IVirtualMachine* vm,
                                  RE::VMStackID stackId,
                                  RE::StaticFunctionTag*,
                                  RE::Actor* targetActor, SInt32 type,
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
}

namespace {
RE::ExtraDataList* CreateExtraDataList()
{
  auto extraList = new RE::ExtraDataList;

  auto extraList_ = reinterpret_cast<BaseExtraList*>(extraList);

  auto p = reinterpret_cast<uint8_t*>(Heap_Allocate(0x18));
  for (int i = 0; i < 0x18; ++i) {
    p[i] = 0;
  }
  reinterpret_cast<void*&>(extraList_->m_presence) = p;

  return extraList;
}
}

namespace {
thread_local bool g_worn = false;
thread_local bool g_wornLeft = false;
}

void TESModPlatform::PushWornState(RE::BSScript::IVirtualMachine* vm,
                                   RE::VMStackID stackId,
                                   RE::StaticFunctionTag*, bool worn,
                                   bool wornLeft)
{
  g_worn = worn;
  g_wornLeft = wornLeft;
}

class MyBSExtraData
{
public:
  MyBSExtraData() = default;
  virtual ~MyBSExtraData() = default;
  virtual UInt32 GetType(void) = 0;

  MyBSExtraData* next; // 08
};

template <ExtraDataType t>
class MyExtra : public MyBSExtraData
{
public:
  MyExtra() = default;

  virtual ~MyExtra() = default;

  UInt32 GetType() override { return t; }
};

void TESModPlatform::AddItemEx(
  RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
  RE::StaticFunctionTag*, RE::TESObjectREFR* containerRefr, RE::TESForm* item,
  SInt32 countDelta, float health, RE::EnchantmentItem* enchantment,
  SInt32 maxCharge, bool removeEnchantmentOnUnequip, float chargePercent,
  RE::BSFixedString textDisplayData, SInt32 soul, RE::AlchemyItem* poison,
  SInt32 poisonCount)
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
}

void TESModPlatform::UpdateEquipment(RE::BSScript::IVirtualMachine* vm,
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
}

void TESModPlatform::ResetContainer(RE::BSScript::IVirtualMachine* vm,
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
}

void TESModPlatform::BlockPapyrusEvents(RE::BSScript::IVirtualMachine* vm,
                                        RE::VMStackID stackId,
                                        RE::StaticFunctionTag*, bool blocked)
{
  papyrusEventsBlocked = blocked;
}

int TESModPlatform::GetWeapDrawnMode(uint32_t actorId)
{
  std::lock_guard l(share.m);
  auto it = share.weapDrawnMode.find(actorId);
  return it == share.weapDrawnMode.end() ? WEAP_DRAWN_MODE_DEFAULT
                                         : it->second;
}

void TESModPlatform::Update()
{
  if (!vmCallAllowed)
    return;
  vmCallAllowed = false;

  papyrusUpdateAllowed = true;

  auto console = RE::ConsoleLog::GetSingleton();
  if (!console)
    return;

  auto vm = RE::SkyrimVM::GetSingleton();
  if (!vm || !vm->impl)
    return console->Print("VM was nullptr");

  FunctionArguments args;
  RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> functor(
    new StackCallbackFunctor);

  RE::BSFixedString className("TESModPlatform");
  RE::BSFixedString funcName("Add");
  vm->impl->DispatchStaticCall(className, funcName, &args, functor);
}

uint64_t TESModPlatform::GetNumPapyrusUpdates()
{
  return numPapyrusUpdates;
}

std::shared_ptr<RE::BSTArray<RE::TintMask*>> TESModPlatform::GetTintsFor(
  uint32_t actorId)
{
  if (actorId < 0xff000000)
    return nullptr;

  std::lock_guard l(share2.m);
  auto i = actorId - 0xff000000;
  if (i >= share2.actorsTints.size())
    return nullptr;
  return share2.actorsTints[i];
}

bool TESModPlatform::GetPapyrusEventsBlocked()
{
  return papyrusEventsBlocked;
}

bool TESModPlatform::Register(RE::BSScript::IVirtualMachine* vm)
{
  TESModPlatform::onPapyrusUpdate = onPapyrusUpdate;

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(Add), SInt32,
                                     RE::StaticFunctionTag*, SInt32, SInt32,
                                     SInt32, SInt32, SInt32, SInt32, SInt32,
                                     SInt32, SInt32, SInt32, SInt32, SInt32>(
      "Add", "TESModPlatform", Add));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<
      true, decltype(MoveRefrToPosition), void, RE::StaticFunctionTag*,
      RE::TESObjectREFR*, RE::TESObjectCELL*, RE::TESWorldSpace*, float, float,
      float, float, float, float>("MoveRefrToPosition", "TESModPlatform",
                                  MoveRefrToPosition));
  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetWeaponDrawnMode), void,
                                     RE::StaticFunctionTag*, RE::Actor*, int>(
      "SetWeaponDrawnMode", "TESModPlatform", SetWeaponDrawnMode));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(GetNthVtableElement),
                                     SInt32, RE::StaticFunctionTag*,
                                     RE::TESForm*, int, int>(
      "GetNthVtableElement", "TESModPlatform", GetNthVtableElement));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(IsPlayerRunningEnabled),
                                     bool, RE::StaticFunctionTag*>(
      "IsPlayerRunningEnabled", "TESModPlatform", IsPlayerRunningEnabled));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(GetSkinColor),
                                     RE::BGSColorForm*, RE::StaticFunctionTag*,
                                     RE::TESNPC*>(
      "GetSkinColor", "TESModPlatform", GetSkinColor));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(CreateNpc), RE::TESNPC*,
                                     RE::StaticFunctionTag*>(
      "CreateNpc", "TESModPlatform", CreateNpc));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcSex), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     SInt32>("SetNpcSex", "TESModPlatform",
                                             SetNpcSex));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcRace), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     RE::TESRace*>(
      "SetNpcRace", "TESModPlatform", SetNpcRace));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcSkinColor), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     SInt32>(
      "SetNpcSkinColor", "TESModPlatform", SetNpcSkinColor));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcHairColor), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     SInt32>(
      "SetNpcHairColor", "TESModPlatform", SetNpcHairColor));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(ResizeHeadpartsArray),
                                     void, RE::StaticFunctionTag*, RE::TESNPC*,
                                     SInt32>(
      "ResizeHeadpartsArray", "TESModPlatform", ResizeHeadpartsArray));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(ResizeTintsArray), void,
                                     RE::StaticFunctionTag*, SInt32>(
      "ResizeTintsArray", "TESModPlatform", ResizeTintsArray));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetFormIdUnsafe), void,
                                     RE::StaticFunctionTag*, RE::TESForm*,
                                     SInt32>(
      "SetFormIdUnsafe", "TESModPlatform", SetFormIdUnsafe));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(ClearTintMasks), void,
                                     RE::StaticFunctionTag*, RE::Actor*>(
      "ClearTintMasks", "TESModPlatform", ClearTintMasks));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(PushTintMask), void,
                                     RE::StaticFunctionTag*, RE::Actor*,
                                     SInt32, UInt32, RE::BSFixedString>(
      "PushTintMask", "TESModPlatform", PushTintMask));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<
      true, decltype(AddItemEx), void, RE::StaticFunctionTag*,
      RE::TESObjectREFR*, RE::TESForm*, SInt32, float, RE::EnchantmentItem*,
      SInt32, bool, float, RE::BSFixedString, SInt32, RE::AlchemyItem*,
      SInt32>("AddItemEx", "TESModPlatform", AddItemEx));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(UpdateEquipment), void,
                                     RE::StaticFunctionTag*, RE::Actor*,
                                     RE::TESForm*, bool>(
      "UpdateEquipment", "TESModPlatform", UpdateEquipment));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(PushWornState), void,
                                     RE::StaticFunctionTag*, bool, bool>(
      "PushWornState", "TESModPlatform", PushWornState));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(ResetContainer), void,
                                     RE::StaticFunctionTag*, RE::TESForm*>(
      "ResetContainer", "TESModPlatform", ResetContainer));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(BlockPapyrusEvents), void,
                                     RE::StaticFunctionTag*, bool>(
      "BlockPapyrusEvents", "TESModPlatform", BlockPapyrusEvents));

  static LoadGameEvent loadGameEvent;

  return true;
}