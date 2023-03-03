#include "PapyrusTESModPlatform.h"
#include "CallNativeApi.h"
#include "ConsoleApi.h"
#include "ExceptionPrinter.h"
#include "NullPointerException.h"

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace TESModPlatform {
bool papyrusUpdateAllowed = false;
bool vmCallAllowed = true;
std::atomic<bool> moveRefrBlocked = false;
std::function<void(IVM* vm, StackID stackId)> onPapyrusUpdate = nullptr;
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
template <class T>
[[nodiscard]] T* CreateForm()
{
  auto form = RE::IFormFactory::GetConcreteFormFactoryByType<T>()->Create();
  return form ? form->As<T>() : nullptr;
}

RE::BSTArray<RE::TintMask*> Clone(const RE::BSTArray<RE::TintMask*>& original)
{
  RE::BSTArray<RE::TintMask*> res;
  for (auto& tint : original) {
    res.push_back(nullptr);
    res.back() = RE::malloc<RE::TintMask>(sizeof(::TintMask));
    memcpy(res.back(), tint, sizeof(::TintMask));
  }
  return res;
}
}

class FunctionArguments : public RE::BSScript::IFunctionArguments
{
public:
  bool operator()(RE::BSScrapArray<Variable>& a_dst) const override
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
  void operator()(Variable a_result) override {}
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
    if (!holder) {
      throw NullPointerException("holder");
    }

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

int32_t TESModPlatform::Add(IVM* vm, StackID stackId, RE::StaticFunctionTag*,
                            int32_t, int32_t, int32_t, int32_t, int32_t,
                            int32_t, int32_t, int32_t, int32_t, int32_t,
                            int32_t, int32_t)
{
  if (!papyrusUpdateAllowed)
    return 0;
  papyrusUpdateAllowed = false;

  try {
    ++numPapyrusUpdates;
    if (!onPapyrusUpdate) {
      throw NullPointerException("onPapyrusUpdate");
    }
    onPapyrusUpdate(vm, stackId);

  } catch (std::exception& e) {
    if (auto console = RE::ConsoleLog::GetSingleton()) {
      console->Print("Papyrus context exception: %s", e.what());
    }
  }

  vmCallAllowed = true;

  return 0;
}

void TESModPlatform::MoveRefrToPosition(
  IVM* vm, StackID stackId, RE::StaticFunctionTag*, RE::TESObjectREFR* refr,
  RE::TESObjectCELL* cell, RE::TESWorldSpace* world, float posX, float posY,
  float posZ, float rotX, float rotY, float rotZ)
{
  if (!refr || (!cell && !world) || moveRefrBlocked) {
    return;
  }

  auto handle = Offsets::GetInvalidRefHandle();
  RE::NiPoint3 pos = { posX, posY, posZ }, rot = { rotX, rotY, rotZ };
  refr->MoveTo_Impl(handle, cell, world, pos, rot);
}

void TESModPlatform::BlockMoveRefrToPosition(bool blocked)
{
  moveRefrBlocked = blocked;
}

void TESModPlatform::SetWeaponDrawnMode(IVM* vm, StackID stackId,
                                        RE::StaticFunctionTag*,
                                        RE::Actor* actor,
                                        int32_t weapDrawnMode)
{
  if (!actor || weapDrawnMode < WEAP_DRAWN_MODE_MIN ||
      weapDrawnMode > WEAP_DRAWN_MODE_MAX) {
    return;
  }

  if (g_nativeCallRequirements.gameThrQ) {
    auto formId = actor->formID;
    g_nativeCallRequirements.gameThrQ->AddTask([=] {
      // kinda redundant since we get formid from actor
      if (RE::TESForm::LookupByID<RE::Actor>(formId) != actor) {
        return;
      }

      if (!actor->IsWeaponDrawn() &&
          weapDrawnMode == WEAP_DRAWN_MODE_ALWAYS_TRUE) {
        actor->DrawWeaponMagicHands(true);
      }

      if (actor->IsWeaponDrawn() &&
          weapDrawnMode == WEAP_DRAWN_MODE_ALWAYS_FALSE) {
        actor->DrawWeaponMagicHands(false);
      }
    });
  }

  std::lock_guard l(share.m);
  share.weapDrawnMode[actor->formID] = weapDrawnMode;
}

int32_t TESModPlatform::GetNthVtableElement(IVM* vm, StackID stackId,
                                            RE::StaticFunctionTag*,
                                            RE::TESForm* pointer,
                                            int32_t pointerOffset,
                                            int32_t elementIndex)
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
        Offsets::BaseAddress;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
  return -1;
}

bool TESModPlatform::IsPlayerRunningEnabled(IVM* vm, StackID stackId,
                                            RE::StaticFunctionTag*)
{
  if (auto controls = RE::PlayerControls::GetSingleton())
    return controls->data.running;
  return false;
}

RE::BGSColorForm* TESModPlatform::GetSkinColor(IVM* vm, StackID stackId,
                                               RE::StaticFunctionTag*,
                                               RE::TESNPC* base)
{
  auto col = CreateForm<RE::BGSColorForm>();
  if (!col) {
    return nullptr;
  }
  col->color = base->bodyTintColor;
  return col;
}

RE::TESNPC* TESModPlatform::CreateNpc(IVM* vm, StackID stackId,
                                      RE::StaticFunctionTag*)
{
  auto npc = CreateForm<RE::TESNPC>();
  if (!npc) {
    return nullptr;
  }

  enum
  {
    AADeleteWhenDoneTestJeremyRegular = 0x0010D13E
  };
  const auto srcNpc =
    RE::TESForm::LookupByID<RE::TESNPC>(AADeleteWhenDoneTestJeremyRegular);
  assert(srcNpc);
  assert(srcNpc->formType.get() == RE::FormType::NPC);
  if (!srcNpc || srcNpc->formType != RE::FormType::NPC) {
    return nullptr;
  }
  auto backup = npc->formID;
  memcpy(npc, srcNpc, sizeof RE::TESNPC);
  npc->formID = backup;

  auto npc_ = npc;
  npc_->numContainerObjects = 0;
  npc_->containerObjects = nullptr;
  npc_->crimeFaction = nullptr;
  npc_->faceNPC = nullptr;
  npc_->actorData.actorBaseFlags.set(RE::ACTOR_BASE_DATA::Flag::kPCLevelMult);
  npc_->actorData.actorBaseFlags.set(RE::ACTOR_BASE_DATA::Flag::kUnique);
  npc_->actorData.actorBaseFlags.set(RE::ACTOR_BASE_DATA::Flag::kSimpleActor);

  // Clear AI Packages to prevent idle animations with Furniture
  enum
  {
    DoNothing = 0x654e2,
    DefaultMoveToCustom02IgnoreCombat = 0x6af62
  };
  // ignore combat && no
  auto doNothing = RE::TESForm::LookupByID<RE::TESPackage>(DoNothing);
  // combat alert
  auto flagsSource =
    RE::TESForm::LookupByID<RE::TESPackage>(DefaultMoveToCustom02IgnoreCombat);

  doNothing->packData = flagsSource->packData;
  npc_->aiPackages.packages.clear();
  npc_->aiPackages.packages.push_front(doNothing);

  auto sourceFaceData = npc_->faceData;
  npc_->faceData = new RE::TESNPC::FaceData;
  if (!npc_->faceData) {
    return nullptr;
  }
  *npc_->faceData = *sourceFaceData;

  return npc;
}

void TESModPlatform::SetNpcSex(IVM* vm, StackID stackId,
                               RE::StaticFunctionTag*, RE::TESNPC* npc,
                               int32_t sex)
{
  if (npc) {
    if (sex == 1) {
      npc->actorData.actorBaseFlags.set(RE::ACTOR_BASE_DATA::Flag::kFemale);
    } else {
      npc->actorData.actorBaseFlags.reset(RE::ACTOR_BASE_DATA::Flag::kFemale);
    }
  }
}

void TESModPlatform::SetNpcRace(IVM* vm, StackID stackId,
                                RE::StaticFunctionTag*, RE::TESNPC* npc,
                                RE::TESRace* race)
{
  if (npc && race) {
    npc->race = race;
  }
}

void TESModPlatform::SetNpcSkinColor(IVM* vm, StackID stackId,
                                     RE::StaticFunctionTag*, RE::TESNPC* npc,
                                     int32_t color)
{
  if (!npc) {
    return;
  }
  npc->bodyTintColor.red = COLOR_RED(color);
  npc->bodyTintColor.green = COLOR_GREEN(color);
  npc->bodyTintColor.blue = COLOR_BLUE(color);
}

void TESModPlatform::SetNpcHairColor(IVM* vm, StackID stackId,
                                     RE::StaticFunctionTag*, RE::TESNPC* npc,
                                     int32_t color)
{
  auto colorForm = CreateForm<RE::BGSColorForm>();
  colorForm->color.red = COLOR_RED(color);
  colorForm->color.green = COLOR_GREEN(color);
  colorForm->color.blue = COLOR_BLUE(color);

  npc->SetHairColor(colorForm);
}

void TESModPlatform::ResizeHeadpartsArray(IVM* vm, StackID stackId,
                                          RE::StaticFunctionTag*,
                                          RE::TESNPC* npc, int8_t newSize)
{
  if (!npc)
    return;
  if (newSize <= 0) {
    npc->headParts = nullptr;
    npc->numHeadParts = 0;
  } else {
    npc->headParts = new RE::BGSHeadPart*[newSize];
    npc->numHeadParts = newSize;

    for (int8_t i = 0; i < npc->numHeadParts; ++i) {
      npc->headParts[i] = nullptr;
    }
  }
}

void TESModPlatform::ResizeTintsArray(IVM* vm, StackID stackId,
                                      RE::StaticFunctionTag*, int32_t newSize)
{
  auto pc = RE::PlayerCharacter::GetSingleton();
  if (!pc) {
    return;
  }

  auto prevSize = pc->tintMasks.size();

  if (newSize < 0 || newSize > 1024 || newSize == prevSize) {
    return;
  }

  pc->tintMasks.resize(newSize);
  for (auto& mask : pc->tintMasks) {
    mask = (RE::TintMask*)new ::TintMask;
  }
}

void TESModPlatform::SetFormIdUnsafe(IVM* vm, StackID stackId,
                                     RE::StaticFunctionTag*, RE::TESForm* form,
                                     uint32_t newId)
{
  if (form) {
    form->formID = newId;
  }
}

void TESModPlatform::ClearTintMasks(IVM* vm, StackID stackId,
                                    RE::StaticFunctionTag*,
                                    RE::Actor* targetActor)
{
  if (!targetActor) {
    auto pc = RE::PlayerCharacter::GetSingleton();
    return pc->tintMasks.clear();
  }

  if (targetActor->formID < 0xff000000) {
    return;
  }
  size_t i = targetActor->formID - 0xff000000;

  std::lock_guard l(share2.m);
  if (share2.actorsTints.size() > i)
    share2.actorsTints[i].reset();
}

void TESModPlatform::PushTintMask(RE::BSScript::IVirtualMachine* vm,
                                  RE::VMStackID stackId,
                                  RE::StaticFunctionTag*,
                                  RE::Actor* targetActor, int32_t type,
                                  uint32_t argb, RE::BSFixedString texturePath)
{
  auto newTm = RE::malloc<::TintMask>();
  if (!newTm)
    return;

  float alpha = COLOR_ALPHA(argb) / 255.f;
  if (alpha > 1.0)
    alpha = 1.f;
  if (alpha < 0.0)
    alpha = 0.f;
  newTm->color.alpha = COLOR_ALPHA(argb);
  newTm->alpha = alpha;
  newTm->color.red = COLOR_RED(argb);
  newTm->color.green = COLOR_GREEN(argb);
  newTm->color.blue = COLOR_BLUE(argb);

  newTm->texture = RE::malloc<RE::TESTexture>();
  if (!newTm->texture)
    return;

  // Not even trying to copy BSFixedString since we do not construct
  // BSFixedString properly
  reinterpret_cast<const char*&>(newTm->texture->textureName) =
    texturePath.c_str();

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
thread_local bool g_worn = false;
thread_local bool g_wornLeft = false;
}

void TESModPlatform::PushWornState(IVM* vm, StackID stackId,
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
  virtual uint32_t GetType(void) = 0;

  MyBSExtraData* next; // 08
};

template <RE::ExtraDataType t>
class MyExtra : public MyBSExtraData
{
public:
  MyExtra() = default;

  virtual ~MyExtra() = default;

  uint32_t GetType() override { return static_cast<uint32_t>(t); }
};

namespace {
RE::ExtraDataList* CreateExtraDataList()
{
  auto extraList = new RE::ExtraDataList;

  auto p = reinterpret_cast<uint8_t*>(RE::malloc(0x18));
  for (int i = 0; i < 0x18; ++i) {
    p[i] = 0;
  }
#ifdef SKYRIMSE
  reinterpret_cast<void*&>(extraList->_presence) = p;
#else
  reinterpret_cast<void*&>(extraList->_extraData.presence) = p;
#endif

  return extraList;
}
}

void TESModPlatform::AddItemEx(
  IVM* vm, StackID stackId, RE::StaticFunctionTag*,
  RE::TESObjectREFR* containerRefr, RE::TESForm* item, int32_t countDelta,
  float health, RE::EnchantmentItem* enchantment, int32_t maxCharge,
  bool removeEnchantmentOnUnequip, float chargePercent,
  FixedString textDisplayData, int32_t soul, RE::AlchemyItem* poison,
  int32_t poisonCount)
{
#ifdef SKYRIMSE
  auto markType = [](RE::ExtraDataList::PresenceBitfield* presence,
#else
  auto markType = [](RE::BaseExtraList::PresenceBitfield* presence,
#endif
                     uint32_t type, bool bCleared) {
    uint32_t index = (type >> 3);
    uint8_t bitMask = 1 << (type % 8);
    uint8_t& flag = presence->bits[index];
    if (bCleared) {
      flag &= ~bitMask;
    } else {
      flag |= bitMask;
    }
  };

  auto addExtra = [markType](void* this__, uint32_t extraType,
                             RE::BSExtraData* toAdd) {
    auto this_ = reinterpret_cast<RE::ExtraDataList*>(this__);

    if (!toAdd || this_->HasType(static_cast<RE::ExtraDataType>(extraType))) {
      return false;
    }

    RE::BSWriteLockGuard locker(this_->_lock);
#ifdef SKYRIMSE
    auto* next = this_->_data;
    this_->_data = toAdd;
#else
    auto* next = this_->_extraData.data;
    this_->_extraData.data = toAdd;
#endif
    toAdd->next = next;
#ifdef SKYRIMSE
    markType(this_->_presence, extraType, false);
#else
    markType(this_->_extraData.presence, extraType, false);
#endif
    return true;
  };

  auto ui = RE::UI::GetSingleton();
  if (!containerRefr || !item || !ui || ui->GameIsPaused())
    return;

  const auto refrId = containerRefr->GetFormID();

  auto boundObject = item->As<RE::TESBoundObject>();
  if (!boundObject) {
    return;
  }

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

    auto extraList_ = reinterpret_cast<void*>(extraList);

    if (g_worn) {
      if (isClothes) {
        auto extra = reinterpret_cast<RE::BSExtraData*>(
          new MyExtra<RE::ExtraDataType::kWorn>);
        addExtra(extraList_, static_cast<uint32_t>(RE::ExtraDataType::kWorn),
                 extra);
      }
    }

    if (g_wornLeft) {
      if (isClothes) {
        auto extra = reinterpret_cast<RE::BSExtraData*>(
          new MyExtra<RE::ExtraDataType::kWornLeft>);
        addExtra(extraList_,
                 static_cast<uint32_t>(RE::ExtraDataType::kWornLeft), extra);
      }
    }

    if (health > 1) {
      addExtra(extraList_, static_cast<uint32_t>(RE::ExtraDataType::kHealth),
               new RE::ExtraHealth(health));
    }

    if (enchantment) {
      addExtra(extraList_,
               static_cast<uint32_t>(RE::ExtraDataType::kEnchantment),
               new RE::ExtraEnchantment(enchantment, maxCharge,
                                        removeEnchantmentOnUnequip));
    }

    if (chargePercent > 0) {
      auto extraCharge = new RE::ExtraCharge;
      extraCharge->charge = chargePercent;
      addExtra(extraList_, static_cast<uint32_t>(RE::ExtraDataType::kCharge),
               extraCharge);
    }

    if (strlen(textDisplayData.data()) > 0) {
      addExtra(extraList_,
               static_cast<uint32_t>(RE::ExtraDataType::kTextDisplayData),
               new RE::ExtraTextDisplayData(textDisplayData.data()));
    }

    if (soul > 0 && soul <= 5) {
      addExtra(extraList_, static_cast<uint32_t>(RE::ExtraDataType::kSoul),
               new RE::ExtraSoul(static_cast<RE::SOUL_LEVEL>(soul)));
    }

    if (poison) {
      addExtra(extraList_, static_cast<uint32_t>(RE::ExtraDataType::kPoison),
               new RE::ExtraPoison(poison, poisonCount));
    }
  }

  g_nativeCallRequirements.gameThrQ->AddTask([=] {
    if (containerRefr != RE::TESForm::LookupByID<RE::TESObjectREFR>(refrId))
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
      static const auto g_bothHandsSlot = RE::TESForm::LookupByID(BothHands);

      auto actor = reinterpret_cast<RE::Actor*>(containerRefr);
      if (s) {
        auto om = RE::BGSDefaultObjectManager::GetSingleton();
        auto slot = reinterpret_cast<RE::BGSEquipSlot*>(
          RE::TESForm::LookupByID(RightHand));

        if (g_wornLeft && !needEquipShieldLike) // wornLeft + shield = deadlock
          slot = reinterpret_cast<RE::BGSEquipSlot*>(
            RE::TESForm::LookupByID(LeftHand));

        if (item->formType == RE::FormType::Ammo) {
          extraList = nullptr;
          slot = nullptr;
        }

        if (countDelta > 0) {
          g_lastEquippedExtraList[g_worn ? false : true][tuple] = extraList;
          g_nativeCallRequirements.gameThrQ->AddTask([=] {
            if (actor != (void*)RE::TESForm::LookupByID(refrId))
              return;
            s->EquipObject(actor, boundObject, extraList, 1, slot);
          });
        } else if (countDelta < 0)
          g_nativeCallRequirements.gameThrQ->AddTask([=] {
            if (actor != (void*)RE::TESForm::LookupByID(refrId))
              return;
            s->UnequipObject(actor, boundObject, extraList, 1, slot);
          });
      }
    }
  }

  g_worn = false;
  g_wornLeft = false;
}

void TESModPlatform::UpdateEquipment(IVM* vm, StackID stackId,
                                     RE::StaticFunctionTag*, RE::Actor* actor,
                                     RE::TESForm* item, bool leftHand)
{

  if (!actor || !actor->currentProcess) {
    return;
  }
  auto ref = leftHand ? actor->currentProcess->GetEquippedLeftHand()
                      : actor->currentProcess->GetEquippedRightHand();
  const auto backup = ref;

  ref = item;
}

void TESModPlatform::ResetContainer(IVM* vm, StackID stackId,
                                    RE::StaticFunctionTag*, RE::TESForm* form)
{
  if (!form) {
    return;
  }

  auto pContainer = form->As<RE::TESContainer>();
  if (!pContainer) {
    return;
  }

  pContainer->numContainerObjects = 0;
  pContainer->containerObjects = nullptr;
}

void TESModPlatform::BlockPapyrusEvents(IVM* vm, StackID stackId,
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

class PapyrusSourcesChecker
{
public:
  PapyrusSourcesChecker()
  {
    auto missing = GetMissingFiles();
    if (!missing.empty()) {
      std::stringstream ss;
      ss << "Missing files: " << nlohmann::json(missing).dump(2)
         << ", reinstalling SkyrimPlatform or/and SKSE may fix that";
      throw std::runtime_error(ss.str());
    }
  }

  std::vector<std::string> GetMissingFiles()
  {
    std::vector<std::string> missing;
    std::istringstream stream(PAPYRUS_SOURCES);
    std::string tmp;
    while (std::getline(stream, tmp, ' ')) {
      tmp.replace(tmp.begin() + tmp.find(".psc"), tmp.end(), ".pex");
      std::filesystem::path path;
      path /= "Data";
      path /= "Scripts";
      path /= tmp;
      if (!std::filesystem::exists(path)) {
        // WorldSpace doesn't have any functions declared so isn't required
        if (tmp != "WorldSpace.pex") {
          missing.push_back(tmp);
        }
      }
    }
    std::sort(missing.begin(), missing.end());
    return missing;
  }
};

void TESModPlatform::Update()
{
  if (!vmCallAllowed) {
    return;
  }
  vmCallAllowed = false;

  papyrusUpdateAllowed = true;

  auto console = RE::ConsoleLog::GetSingleton();
  if (!console) {
    return;
  }

  auto vm = RE::SkyrimVM::GetSingleton();
  if (!vm || !vm->impl) {
    return console->Print("VM was nullptr");
  }

  FunctionArguments args;
  RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> functor(
    new StackCallbackFunctor);

  // Prevents calling DispatchStaticCall by throwing an exception if we know
  // it's going to crash
  try {
    static PapyrusSourcesChecker checker;

    // DispatchStaticCall is gonna crash if TESModPlatform.pex or any of its
    // dependencies (like Actor.pex) is missing
    FixedString className("TESModPlatform");
    FixedString funcName("Add");
    vm->impl->DispatchStaticCall(className, funcName, &args, functor);
  } catch (std::exception& e) {
    // We are not interested in crashing the game thread, so just printing
    static std::once_flag flag;
    std::call_once(flag, [&] { ExceptionPrinter::Print(e); });
  }
}

uint64_t TESModPlatform::GetNumPapyrusUpdates()
{
  return numPapyrusUpdates;
}

std::shared_ptr<RE::BSTArray<RE::TintMask*>> TESModPlatform::GetTintsFor(
  uint32_t actorId)
{
  if (actorId < 0xff000000) {
    return nullptr;
  }

  std::lock_guard l(share2.m);
  auto i = actorId - 0xff000000;
  if (i >= share2.actorsTints.size()) {
    return nullptr;
  }
  return share2.actorsTints[i];
}

bool TESModPlatform::GetPapyrusEventsBlocked()
{
  return papyrusEventsBlocked;
}

bool TESModPlatform::Register(IVM* vm)
{
  TESModPlatform::onPapyrusUpdate = onPapyrusUpdate;

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<
      true, decltype(Add), int32_t, RE::StaticFunctionTag*, int32_t, int32_t,
      int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
      int32_t, int32_t>("Add", "TESModPlatform", Add));

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
                                     int32_t, RE::StaticFunctionTag*,
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
                                     int32_t>("SetNpcSex", "TESModPlatform",
                                              SetNpcSex));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcRace), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     RE::TESRace*>(
      "SetNpcRace", "TESModPlatform", SetNpcRace));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcSkinColor), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     int32_t>(
      "SetNpcSkinColor", "TESModPlatform", SetNpcSkinColor));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcHairColor), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     int32_t>(
      "SetNpcHairColor", "TESModPlatform", SetNpcHairColor));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(ResizeHeadpartsArray),
                                     void, RE::StaticFunctionTag*, RE::TESNPC*,
                                     int32_t>(
      "ResizeHeadpartsArray", "TESModPlatform", ResizeHeadpartsArray));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(ResizeTintsArray), void,
                                     RE::StaticFunctionTag*, int32_t>(
      "ResizeTintsArray", "TESModPlatform", ResizeTintsArray));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetFormIdUnsafe), void,
                                     RE::StaticFunctionTag*, RE::TESForm*,
                                     int32_t>(
      "SetFormIdUnsafe", "TESModPlatform", SetFormIdUnsafe));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(ClearTintMasks), void,
                                     RE::StaticFunctionTag*, RE::Actor*>(
      "ClearTintMasks", "TESModPlatform", ClearTintMasks));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(PushTintMask), void,
                                     RE::StaticFunctionTag*, RE::Actor*,
                                     int32_t, uint32_t, FixedString>(
      "PushTintMask", "TESModPlatform", PushTintMask));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<
      true, decltype(AddItemEx), void, RE::StaticFunctionTag*,
      RE::TESObjectREFR*, RE::TESForm*, int32_t, float, RE::EnchantmentItem*,
      int32_t, bool, float, FixedString, int32_t, RE::AlchemyItem*, int32_t>(
      "AddItemEx", "TESModPlatform", AddItemEx));

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
