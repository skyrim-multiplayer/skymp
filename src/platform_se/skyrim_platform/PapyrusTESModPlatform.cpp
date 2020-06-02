#include "PapyrusTESModPlatform.h"
#include "CallNativeApi.h"
#include "NullPointerException.h"
#include <RE/BSScript/IFunctionArguments.h>
#include <RE/BSScript/IStackCallbackFunctor.h>
#include <RE/BSScript/NativeFunction.h>
#include <RE/ConsoleLog.h>
#include <RE/PlayerControls.h>
#include <RE/ScriptEventSourceHolder.h>
#include <RE/SkyrimVM.h>
#include <mutex>
#include <skse64/GameReferences.h>
#include <unordered_map>

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace TESModPlatform {
bool papyrusUpdateAllowed = false;
bool vmCallAllowed = true;
std::function<void(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId)>
  onPapyrusUpdate = nullptr;
uint64_t numPapyrusUpdates = 0;
struct
{
  std::unordered_map<uint32_t, int> weapDrawnMode;
  std::recursive_mutex m;
} share;

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
  if (!refr || (!cell && !world))
    return;

  NiPoint3 pos = { posX, posY, posZ }, rot = { rotX, rotY, rotZ };
  auto nullHandle = *g_invalidRefHandle;

  auto f = ::MoveRefrToPosition.operator _MoveRefrToPosition();
  f(reinterpret_cast<TESObjectREFR*>(refr), &nullHandle, cell, world, &pos,
    &rot);
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

  static LoadGameEvent loadGameEvent;

  return true;
}