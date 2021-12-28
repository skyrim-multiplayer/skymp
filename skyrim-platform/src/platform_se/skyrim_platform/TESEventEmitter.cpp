#include "TESEventEmitter.h"

// Testing Includes
#include <RE/ConsoleLog.h>
#include <RE/PlayerCharacter.h>
#include <RE/TESActiveEffectApplyRemoveEvent.h>
#include <RE/TESCellFullyLoadedEvent.h>
#include <RE/TESCombatEvent.h>
#include <RE/TESContainerChangedEvent.h>
#include <RE/TESDeathEvent.h>
#include <RE/TESEquipEvent.h>
#include <RE/TESGrabReleaseEvent.h>
#include <RE/TESHitEvent.h>
#include <RE/TESInitScriptEvent.h>
#include <RE/TESLoadGameEvent.h>
#include <RE/TESLockChangedEvent.h>
#include <RE/TESMagicEffectApplyEvent.h>
#include <RE/TESMoveAttachDetachEvent.h>
#include <RE/TESObjectCELL.h>
#include <RE/TESObjectLoadedEvent.h>
#include <RE/TESResetEvent.h>
#include <RE/TESSwitchRaceCompleteEvent.h>
#include <RE/TESTrackedStatsEvent.h>
#include <RE/TESUniqueIDChangeEvent.h>
#include <RE/TESWaitStopEvent.h>

template <class T>
void TESEventEmitter::Emit(T* event)
{
  auto sesh = RE::ScriptEventSourceHolder::GetSingleton();
  sesh->SendEvent<T>(event);
}

void TESEventEmitter::HitEvent(RE::TESHitEvent* event)
{
  Emit<RE::TESHitEvent>(event);
}
// not work
void TESEventEmitter::SpellCastEvent(TESEvents::TESSpellCastEvent* event)
{
  Emit<RE::TESSpellCastEvent>(reinterpret_cast<RE::TESSpellCastEvent*>(event));
}

void TESEventEmitter::MoveAttachDetachEvent(
  RE::TESMoveAttachDetachEvent* event)
{
  Emit<RE::TESMoveAttachDetachEvent>(event);
}

void TESEventEmitter::ActivateEvent(RE::TESActivateEvent* event)
{
  Emit<RE::TESActivateEvent>(event);
}

void TESEventEmitter::WaitStopEvent(RE::TESWaitStopEvent* event)
{
  Emit<RE::TESWaitStopEvent>(event);
}

void TESEventEmitter::ObjectLoadedEvent(RE::TESObjectLoadedEvent* event)
{
  Emit<RE::TESObjectLoadedEvent>(event);
}

void TESEventEmitter::LockChangedEvent(RE::TESLockChangedEvent* event)
{
  Emit<RE::TESLockChangedEvent>(event);
}
// not tested
void TESEventEmitter::CellFullyLoadedEvent(RE::TESCellFullyLoadedEvent* event)
{
  Emit<RE::TESCellFullyLoadedEvent>(event);
}

void TESEventEmitter::GrabReleaseEvent(RE::TESGrabReleaseEvent* event)
{
  Emit<RE::TESGrabReleaseEvent>(event);
}

void TESEventEmitter::LoadGameEvent(RE::TESLoadGameEvent* event)
{
  Emit<RE::TESLoadGameEvent>(event);
}

void TESEventEmitter::SwitchRaceCompleteEvent(
  RE::TESSwitchRaceCompleteEvent* event)
{
  Emit<RE::TESSwitchRaceCompleteEvent>(event);
}

void TESEventEmitter::UniqueIDChangeEvent(RE::TESUniqueIDChangeEvent* event)
{
  Emit<RE::TESUniqueIDChangeEvent>(event);
}

void TESEventEmitter::TrackedStatsEvent(RE::TESTrackedStatsEvent* event)
{
  Emit<RE::TESTrackedStatsEvent>(event);
}

void TESEventEmitter::InitScriptEvent(RE::TESInitScriptEvent* event)
{
  Emit<RE::TESInitScriptEvent>(event);
}

void TESEventEmitter::ResetEvent(RE::TESResetEvent* event)
{
  Emit<RE::TESResetEvent>(event);
}

void TESEventEmitter::CombatEvent(RE::TESCombatEvent* event)
{
  Emit<RE::TESCombatEvent>(event);
}

void TESEventEmitter::DeathEvent(RE::TESDeathEvent* event)
{
  Emit<RE::TESDeathEvent>(event);
}

void TESEventEmitter::ContainerChangedEvent(
  RE::TESContainerChangedEvent* event)
{
  Emit<RE::TESContainerChangedEvent>(event);
}

void TESEventEmitter::EquipEvent(RE::TESEquipEvent* event)
{
  Emit<RE::TESEquipEvent>(event);
}

void TESEventEmitter::ActiveEffectApplyRemoveEvent(
  RE::TESActiveEffectApplyRemoveEvent* event)
{
  Emit<RE::TESActiveEffectApplyRemoveEvent>(event);
}

void TESEventEmitter::MagicEffectApplyEvent(
  RE::TESMagicEffectApplyEvent* event)
{
  Emit<RE::TESMagicEffectApplyEvent>(event);
}

// Test method
void TESEventEmitter::TestEvent()
{
  auto playerRef =
    static_cast<RE::TESObjectREFR*>(RE::PlayerCharacter::GetSingleton());
  auto playerNi = static_cast<RE::NiPointer<RE::TESObjectREFR>>(playerRef);
  auto form = static_cast<RE::FormID>(20);

  if (auto c = RE::ConsoleLog::GetSingleton()) {
    c->Print("Fire Test Event");
  }
  // Test Hit
  // TESEventEmitter::HitEvent(new RE::TESHitEvent(
  //   playerRef, playerRef, form,
  //   form, RE::TESHitEvent::Flag::kHitBlocked));

  // Test SpellCast - not work
  auto obj = new TESEvents::TESSpellCastEvent();
  obj->caster = playerNi;
  obj->spell = form;
  TESEventEmitter::SpellCastEvent(obj);

  // Test MoveAttachDetach
  // auto obj = new RE::TESMoveAttachDetachEvent();
  // obj->movedRef = playerNi;
  // obj->isCellAttached = false;
  // TESEventEmitter::MoveAttachDetachEvent(obj);

  // Test ActivateEvent - not tested
  // auto obj = new TESEvents::TESActivateEvent();
  // obj->target = playerNi;
  // obj->caster = playerNi;
  // TESEventEmitter::ActivateEvent(reinterpret_cast<RE::TESActivateEvent*>(obj));

  // Test WaitStop
  // auto obj = new RE::TESWaitStopEvent();
  // obj->interrupted = true;
  // TESEventEmitter::WaitStopEvent(obj);

  // Test TESObjectLoadedEvent
  // auto obj = new RE::TESObjectLoadedEvent();
  // obj->formID = form;
  // obj->loaded = true;
  // TESEventEmitter::ObjectLoadedEvent(obj);

  // Test TESLockChangedEvent
  // auto obj = new RE::TESLockChangedEvent();
  // obj->lockedObject = playerNi;
  // TESEventEmitter::LockChangedEvent(obj);

  // Test TESCellFullyLoadedEvent - not tested
  // auto obj = new RE::TESCellFullyLoadedEvent();
  // obj->cell = new RE::TESObjectCELL();
  // TESEventEmitter::CellFullyLoadedEvent(obj);

  // Test GrabRelease
  // auto obj = new RE::TESGrabReleaseEvent();
  // obj->grabbed = true;
  // obj->ref = playerNi;
  // TESEventEmitter::GrabReleaseEvent(obj);

  // Test LoadGame
  // auto obj2 = new RE::TESLoadGameEvent();
  // TESEventEmitter::LoadGameEvent(obj2);

  // Test TESSwitchRaceCompleteEvent
  // auto obj = new RE::TESSwitchRaceCompleteEvent();
  // obj->subject = playerNi;
  // TESEventEmitter::SwitchRaceCompleteEvent(obj);

  // Test TESUniqueIDChangeEvent
  // auto obj2 = new RE::TESUniqueIDChangeEvent();
  // obj2->newBaseID = form;
  // obj2->newUniqueID = static_cast<UInt16>(5);
  // obj2->objectID = form;
  // obj2->oldBaseID = form;
  // obj2->oldUniqueID = static_cast<UInt16>(8);
  // TESEventEmitter::UniqueIDChangeEvent(obj2);

  // Test TESTrackedStatsEvent
  // auto obj3 = new RE::TESTrackedStatsEvent();
  // obj3->stat = static_cast<RE::BSFixedString>("Hey you");
  // obj3->value = static_cast<SInt32>(6);
  // TESEventEmitter::TrackedStatsEvent(obj3);

  // Test TESInitScriptEvent
  // auto obj4 = new RE::TESInitScriptEvent();
  // obj4->objectInitialized = playerNi;
  // TESEventEmitter::InitScriptEvent(obj4);

  // Test TESResetEvent
  // auto obj = new RE::TESResetEvent();
  // obj->object = playerNi;
  // TESEventEmitter::ResetEvent(obj);

  // Test TESCombatEvent
  // auto obj2 = new RE::TESCombatEvent();
  // obj2->actor = playerNi;
  // obj2->state = RE::ACTOR_COMBAT_STATE::kCombat;
  // obj2->targetActor = playerNi;
  // TESEventEmitter::CombatEvent(obj2);

  // Test TESDeathEvent
  // auto obj3 = new RE::TESDeathEvent();
  // obj3->actorDying = playerNi;
  // obj3->actorKiller = playerNi;
  // obj3->dead = true;
  // TESEventEmitter::DeathEvent(obj3);

  // Test TESContainerChangedEvent
  // auto obj4 = new RE::TESContainerChangedEvent();
  // obj4->baseObj = form;
  // obj4->itemCount = 2;
  // obj4->newContainer = form;
  // obj4->oldContainer = form;
  // obj4->reference = RE::ObjectRefHandle();
  // obj4->uniqueID = 5;
  // TESEventEmitter::ContainerChangedEvent(obj4);

  // Test TESEquipEvent
  // auto obj = new RE::TESEquipEvent();
  // obj->actor = playerNi;
  // obj->baseObject = form;
  // obj->equipped = true;
  // obj->originalRefr = form;
  // obj->uniqueID = 32;
  // TESEventEmitter::EquipEvent(obj);

  // Test TESActiveEffectApplyRemoveEvent
  // auto obj2 = new RE::TESActiveEffectApplyRemoveEvent();
  // obj2->activeEffectUniqueID = 56;
  // obj2->caster = playerNi;
  // obj2->isApplied = true;
  // obj2->target = playerNi;
  // TESEventEmitter::ActiveEffectApplyRemoveEvent(obj2);

  // Test TESMagicEffectApplyEvent
  // auto obj3 = new RE::TESMagicEffectApplyEvent();
  // obj3->caster = playerNi;
  // obj3->magicEffect = form;
  // obj3->target = playerNi;
  // TESEventEmitter::MagicEffectApplyEvent(obj3);
}
