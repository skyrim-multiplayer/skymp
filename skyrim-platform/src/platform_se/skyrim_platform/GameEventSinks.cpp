#include "GameEventSinks.h"
#include "EventsApi.h"
#include "JsEngine.h"
#include "NativeValueCasts.h"
#include "SkyrimPlatform.h"
#include "TESEvents.h"
#include "TaskQueue.h"
#include <RE/ActiveEffect.h>
#include <RE/Actor.h>
#include <RE/EffectSetting.h>
#include <RE/TESObjectCELL.h>

struct RE::TESActivateEvent
{
  NiPointer<TESObjectREFR> target;
  NiPointer<TESObjectREFR> caster;
};

namespace {
JsValue CreateObject(const char* type, void* form)
{
  return form ? NativeValueCasts::NativeObjectToJsObject(
                  std::make_shared<CallNative::Object>(type, form))
              : JsValue::Null();
}
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESActivateEvent* event,
  RE::BSTEventSource<RE::TESActivateEvent>* eventSource)
{
  auto targetRefr = event ? event->target.get() : nullptr;
  auto casterRefr = event ? event->caster.get() : nullptr;

  auto targetId = targetRefr ? targetRefr->formID : 0;
  auto casterId = casterRefr ? casterRefr->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [targetId, casterId, targetRefr, casterRefr] {
      auto obj = JsValue::Object();

      auto target = RE::TESForm::LookupByID(targetId);
      target = target == targetRefr ? target : nullptr;
      obj.SetProperty("target", CreateObject("ObjectReference", target));

      auto caster = RE::TESForm::LookupByID(casterId);
      caster = caster == casterRefr ? caster : nullptr;
      obj.SetProperty("caster", CreateObject("ObjectReference", caster));

      auto targetRefr_ = reinterpret_cast<RE::TESObjectREFR*>(target);

      obj.SetProperty("isCrimeToActivate",
                      targetRefr_
                        ? JsValue::Bool(targetRefr_->IsCrimeToActivate())
                        : JsValue::Undefined());

      EventsApi::SendEvent("activate", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESMoveAttachDetachEvent* event,
  RE::BSTEventSource<RE::TESMoveAttachDetachEvent>* eventSource)
{
  auto movedRef = event ? event->movedRef.get() : nullptr;

  auto targetId = movedRef ? movedRef->formID : 0;
  auto isCellAttached = event ? event->isCellAttached : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [targetId, isCellAttached, movedRef] {
      auto obj = JsValue::Object();

      auto target = RE::TESForm::LookupByID(targetId);
      target = target == movedRef ? target : nullptr;
      obj.SetProperty("movedRef", CreateObject("ObjectReference", target));

      obj.SetProperty("isCellAttached", JsValue::Bool(isCellAttached));
      EventsApi::SendEvent("moveAttachDetach", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESWaitStopEvent* event,
  RE::BSTEventSource<RE::TESWaitStopEvent>* eventSource)
{
  auto interrupted = event ? event->interrupted : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([interrupted] {
    auto obj = JsValue::Object();

    obj.SetProperty("isInterrupted", JsValue::Bool(interrupted));

    EventsApi::SendEvent("waitStop", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESObjectLoadedEvent* event,
  RE::BSTEventSource<RE::TESObjectLoadedEvent>* eventSource)
{
  auto objectId = event ? event->formID : 0;
  auto loaded = event ? event->loaded : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([objectId, loaded] {
    auto obj = JsValue::Object();

    obj.SetProperty("object",
                    CreateObject("Form", RE::TESForm::LookupByID(objectId)));

    obj.SetProperty("isLoaded", JsValue::Bool(loaded));

    EventsApi::SendEvent("objectLoaded", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESLockChangedEvent* event,
  RE::BSTEventSource<RE::TESLockChangedEvent>* eventSource)
{
  auto lockedObject = event ? event->lockedObject : nullptr;
  auto lockedObjectId = lockedObject ? lockedObject->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([lockedObjectId, lockedObject] {
    auto obj = JsValue::Object();

    auto lockedObject = RE::TESForm::LookupByID(lockedObjectId);
    lockedObject = lockedObject == lockedObject ? lockedObject : nullptr;
    obj.SetProperty("lockedObject",
                    CreateObject("ObjectReference", lockedObject));

    EventsApi::SendEvent("lockChanged", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESCellFullyLoadedEvent* event,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>* eventSource)
{
  auto cell = event ? event->cell : nullptr;
  auto cellId = cell ? cell->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([cellId, cell] {
    auto obj = JsValue::Object();

    auto cell_ = RE::TESForm::LookupByID(cellId);
    cell_ = cell_ == cell ? cell_ : nullptr;
    obj.SetProperty("cell", CreateObject("Cell", cell_));

    EventsApi::SendEvent("cellFullyLoaded", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESGrabReleaseEvent* event,
  RE::BSTEventSource<RE::TESGrabReleaseEvent>* eventSource)
{
  auto ref = event ? event->ref.get() : nullptr;
  auto refId = ref ? ref->formID : 0;
  auto grabbed = event ? event->grabbed : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([refId, grabbed, ref] {
    auto obj = JsValue::Object();

    auto reference = RE::TESForm::LookupByID(refId);
    reference = reference == ref ? reference : nullptr;
    obj.SetProperty("refr", CreateObject("ObjectReference", reference));

    obj.SetProperty("isGrabbed", JsValue::Bool(grabbed));

    EventsApi::SendEvent("grabRelease", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESLoadGameEvent* event,
  RE::BSTEventSource<RE::TESLoadGameEvent>* eventSource)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [] { EventsApi::SendEvent("loadGame", { JsValue::Undefined() }); });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESSwitchRaceCompleteEvent* event,
  RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>* eventSource)
{
  auto subject = event ? event->subject.get() : nullptr;
  auto subjectId = subject ? subject->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([subjectId, subject] {
    auto obj = JsValue::Object();

    auto subjectLocal = RE::TESForm::LookupByID(subjectId);
    subjectLocal = subjectLocal == subject ? subjectLocal : nullptr;
    obj.SetProperty("subject", CreateObject("ObjectReference", subjectLocal));

    EventsApi::SendEvent("switchRaceComplete", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESUniqueIDChangeEvent* event,
  RE::BSTEventSource<RE::TESUniqueIDChangeEvent>* eventSource)
{
  auto oldUniqueID = event ? event->oldUniqueID : 0;
  auto newUniqueID = event ? event->newUniqueID : 0;

  auto oldBaseID = event ? event->oldBaseID : 0;
  auto newBaseID = event ? event->newBaseID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [oldUniqueID, newUniqueID, oldBaseID, newBaseID] {
      auto obj = JsValue::Object();

      obj.SetProperty("oldBaseID", JsValue::Double(oldBaseID));
      obj.SetProperty("newBaseID", JsValue::Double(newBaseID));
      obj.SetProperty("oldUniqueID", JsValue::Double(oldUniqueID));
      obj.SetProperty("newUniqueID", JsValue::Double(newUniqueID));

      EventsApi::SendEvent("uniqueIdChange", { JsValue::Undefined(), obj });
    });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESTrackedStatsEvent* event,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>* eventSource)
{
  std::string statName = event ? event->stat.data() : "";
  auto value = event ? event->value : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([statName, value] {
    auto obj = JsValue::Object();

    obj.SetProperty("statName", JsValue::String(statName));
    obj.SetProperty("newValue", JsValue::Double(value));

    EventsApi::SendEvent("trackedStats", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESInitScriptEvent* event,
  RE::BSTEventSource<RE::TESInitScriptEvent>* eventSource)
{
  auto objectInitialized = event ? event->objectInitialized.get() : nullptr;
  auto objectInitializedId = objectInitialized ? objectInitialized->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([objectInitializedId,
                                                objectInitialized] {
    auto obj = JsValue::Object();

    auto objectInitializedLocal = RE::TESForm::LookupByID(objectInitializedId);
    objectInitializedLocal = objectInitializedLocal == objectInitialized
      ? objectInitializedLocal
      : nullptr;

    obj.SetProperty("initializedObject",
                    CreateObject("ObjectReference", objectInitializedLocal));

    EventsApi::SendEvent("scriptInit", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESResetEvent* event,
  RE::BSTEventSource<RE::TESResetEvent>* eventSource)
{
  auto object = event ? event->object.get() : nullptr;
  auto objectId = object ? object->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([objectId, object] {
    auto obj = JsValue::Object();

    auto objectIdLocal = RE::TESForm::LookupByID(objectId);
    objectIdLocal = objectIdLocal == object ? objectIdLocal : nullptr;

    obj.SetProperty("object", CreateObject("ObjectReference", objectIdLocal));

    EventsApi::SendEvent("reset", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kStop;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESCombatEvent* event,
  RE::BSTEventSource<RE::TESCombatEvent>* eventSource)
{
  auto targetActorRefr = event ? event->targetActor.get() : nullptr;
  auto targetActorId = targetActorRefr ? targetActorRefr->formID : 0;

  auto actorRefr = event ? event->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto state = event ? (uint32_t)event->state : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [targetActorId, actorId, state, targetActorRefr, actorRefr] {
      auto obj = JsValue::Object();

      auto targetActorLocal = RE::TESForm::LookupByID(targetActorId);
      targetActorLocal =
        targetActorLocal == targetActorRefr ? targetActorLocal : nullptr;
      obj.SetProperty("target",
                      CreateObject("ObjectReference", targetActorLocal));

      auto actorLocal = RE::TESForm::LookupByID(actorId);
      actorLocal = actorLocal == actorRefr ? actorLocal : nullptr;
      obj.SetProperty("actor", CreateObject("ObjectReference", actorLocal));

      obj.SetProperty(
        "isCombat",
        JsValue::Bool(state & (uint32_t)RE::ACTOR_COMBAT_STATE::kCombat));

      obj.SetProperty(
        "isSearching",
        JsValue::Bool(state & (uint32_t)RE::ACTOR_COMBAT_STATE::kSearching));

      EventsApi::SendEvent("combatState", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESDeathEvent* event,
  RE::BSTEventSource<RE::TESDeathEvent>* eventSource)
{
  auto actorDyingRefr = event ? event->actorDying.get() : nullptr;
  auto actorDyingId = actorDyingRefr ? actorDyingRefr->formID : 0;

  auto actorKillerRefr = event ? event->actorKiller.get() : nullptr;
  auto actorKillerId = actorKillerRefr ? actorKillerRefr->formID : 0;

  auto dead = event ? event->dead : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [actorDyingId, actorKillerId, dead, actorDyingRefr, actorKillerRefr] {
      auto obj = JsValue::Object();

      auto actorDyingLocal = RE::TESForm::LookupByID(actorDyingId);
      actorDyingLocal =
        actorDyingLocal == actorDyingRefr ? actorDyingLocal : nullptr;
      obj.SetProperty("actorDying",
                      CreateObject("ObjectReference", actorDyingLocal));

      auto actorKillerLocal = RE::TESForm::LookupByID(actorKillerId);
      actorKillerLocal =
        actorKillerLocal == actorKillerRefr ? actorKillerLocal : nullptr;
      obj.SetProperty("actorKiller",
                      CreateObject("ObjectReference", actorKillerLocal));

      dead ? EventsApi::SendEvent("deathEnd", { JsValue::Undefined(), obj })
           : EventsApi::SendEvent("deathStart", { JsValue::Undefined(), obj });
    });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESContainerChangedEvent* event,
  RE::BSTEventSource<RE::TESContainerChangedEvent>* eventSource)
{
  auto oldContainerId = event ? event->oldContainer : 0;
  auto newContainerId = event ? event->newContainer : 0;
  auto baseObjId = event ? event->baseObj : 0;
  auto itemCount = event ? event->itemCount : 0;
  auto uniqueID = event ? event->uniqueID : 0;

  auto reference = event ? event->reference.get() : nullptr;
  auto referenceId = reference ? reference->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([oldContainerId, newContainerId,
                                                baseObjId, itemCount, uniqueID,
                                                referenceId] {
    auto obj = JsValue::Object();

    obj.SetProperty("oldContainer",
                    CreateObject("ObjectReference",
                                 RE::TESForm::LookupByID(oldContainerId)));

    obj.SetProperty("newContainer",
                    CreateObject("ObjectReference",
                                 RE::TESForm::LookupByID(newContainerId)));

    obj.SetProperty("baseObj",
                    CreateObject("Form", RE::TESForm::LookupByID(baseObjId)));

    obj.SetProperty("numItems", JsValue::Double(itemCount));
    obj.SetProperty("uniqueID", JsValue::Double(uniqueID));

    obj.SetProperty(
      "reference",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(referenceId)));

    EventsApi::SendEvent("containerChanged", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESHitEvent* event,
  RE::BSTEventSource<RE::TESHitEvent>* eventSource)
{
  auto targetRefr = event ? event->target.get() : nullptr;
  auto causeRefr = event ? event->cause.get() : nullptr;

  auto targetId = targetRefr ? targetRefr->formID : 0;
  auto causeId = causeRefr ? causeRefr->formID : 0;

  auto sourceId = event ? event->source : 0;
  auto projectileId = event ? event->projectile : 0;
  uint8_t flags = event ? (uint8_t)event->flags : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [targetId, causeId, sourceId, projectileId, flags, targetRefr, causeRefr] {
      auto obj = JsValue::Object();

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      targetLocal = targetLocal == targetRefr ? targetLocal : nullptr;
      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));

      auto causeLocal = RE::TESForm::LookupByID(causeId);
      causeLocal = causeLocal == causeRefr ? causeLocal : nullptr;

      // TODO(#336): drop old name "agressor" on next major release of SP
      auto aggressor = CreateObject("ObjectReference", causeLocal);
      obj.SetProperty("agressor", aggressor);
      obj.SetProperty("aggressor", aggressor);

      obj.SetProperty("source",
                      CreateObject("Form", RE::TESForm::LookupByID(sourceId)));

      obj.SetProperty(
        "projectile",
        CreateObject("Form", RE::TESForm::LookupByID(projectileId)));

      obj.SetProperty(
        "isPowerAttack",
        JsValue::Bool(flags & (uint8_t)RE::TESHitEvent::Flag::kPowerAttack));

      obj.SetProperty(
        "isSneakAttack",
        JsValue::Bool(flags & (uint8_t)RE::TESHitEvent::Flag::kSneakAttack));

      obj.SetProperty(
        "isBashAttack",
        JsValue::Bool(flags & (uint8_t)RE::TESHitEvent::Flag::kBashAttack));

      obj.SetProperty(
        "isHitBlocked",
        JsValue::Bool(flags & (uint8_t)RE::TESHitEvent::Flag::kHitBlocked));

      EventsApi::SendEvent("hit", { JsValue::Undefined(), obj });
    });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESEquipEvent* event,
  RE::BSTEventSource<RE::TESEquipEvent>* eventSource)
{
  auto actorRefr = event ? event->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto originalRefrId = event ? event->originalRefr : 0;
  auto baseObjectId = event ? event->baseObject : 0;
  auto equipped = event ? event->equipped : 0;
  auto uniqueId = event ? event->uniqueID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([actorId, baseObjectId,
                                                equipped, uniqueId,
                                                originalRefrId, actorRefr] {
    auto obj = JsValue::Object();

    auto actorLocal = RE::TESForm::LookupByID(actorId);
    actorLocal = actorLocal == actorRefr ? actorLocal : nullptr;
    obj.SetProperty("actor", CreateObject("ObjectReference", actorLocal));

    obj.SetProperty(
      "baseObj", CreateObject("Form", RE::TESForm::LookupByID(baseObjectId)));

    obj.SetProperty("originalRefr",
                    CreateObject("ObjectReference",
                                 RE::TESForm::LookupByID(originalRefrId)));

    obj.SetProperty("uniqueId", JsValue::Double(uniqueId));

    equipped ? EventsApi::SendEvent("equip", { JsValue::Undefined(), obj })
             : EventsApi::SendEvent("unequip", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESActiveEffectApplyRemoveEvent* event,
  RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>* eventSource)
{
  auto caster = event->caster.get() ? event->caster.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  auto isApplied = event ? event->isApplied : 0;
  auto activeEffectUniqueID = event ? event->activeEffectUniqueID : 0;
  RE::ActiveEffect* activeEffect = nullptr;

  auto actor = reinterpret_cast<RE::Actor*>(target);
  if (actor && actor->formType == RE::FormType::ActorCharacter) {
    if (auto effectList = actor->GetActiveEffectList()) {
      for (auto effect : *effectList) {
        if (effect && effect->usUniqueID == activeEffectUniqueID) {
          activeEffect = effect;
          break;
        }
      }
    }
  }

  auto activeEffectBase =
    activeEffect ? activeEffect->GetBaseObject() : nullptr;

  auto activeEffectBaseId = activeEffectBase ? activeEffectBase->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [casterId, targetId, isApplied, activeEffect, activeEffectUniqueID,
     activeEffectBaseId, caster, target] {
      auto obj = JsValue::Object();

      bool isEffectValid = activeEffect
        ? (activeEffect->usUniqueID == activeEffectUniqueID)
        : false;

      obj.SetProperty(
        "effect",
        CreateObject("MagicEffect",
                     RE::TESForm::LookupByID(activeEffectBaseId)));

      obj.SetProperty("activeEffect",
                      CreateObject("ActiveMagicEffect",
                                   isEffectValid ? activeEffect : nullptr));

      auto casterLocal = RE::TESForm::LookupByID(casterId);
      casterLocal = casterLocal == caster ? casterLocal : nullptr;

      obj.SetProperty("caster", CreateObject("ObjectReference", casterLocal));

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      targetLocal = targetLocal == target ? targetLocal : nullptr;

      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));

      isApplied
        ? EventsApi::SendEvent("effectStart", { JsValue::Undefined(), obj })
        : EventsApi::SendEvent("effectFinish", { JsValue::Undefined(), obj });
    });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESMagicEffectApplyEvent* event,
  RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* eventSource)
{
  auto effectId = event ? event->magicEffect : 0;

  auto caster = event->caster.get() ? event->caster.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [effectId, casterId, targetId, caster, target] {
      auto obj = JsValue::Object();

      auto effect = RE::TESForm::LookupByID(effectId);

      if (effect && effect->formType != RE::FormType::MagicEffect)
        return;

      obj.SetProperty("effect", CreateObject("MagicEffect", effect));

      auto casterLocal = RE::TESForm::LookupByID(casterId);
      casterLocal = casterLocal == caster ? casterLocal : nullptr;

      obj.SetProperty("caster", CreateObject("ObjectReference", casterLocal));

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      targetLocal = targetLocal == target ? targetLocal : nullptr;

      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));

      EventsApi::SendEvent("magicEffectApply", { JsValue::Undefined(), obj });
    });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::MenuOpenCloseEvent* e,
  RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource)
{
  const char* menuName = e->menuName.c_str();

  if (e->opening) {
    EventsApi::SendMenuOpen(menuName);
  } else {
    EventsApi::SendMenuClose(menuName);
  }

  return RE::BSEventNotifyControl::kContinue;
};

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESSpellCastEvent* event,
  RE::BSTEventSource<RE::TESSpellCastEvent>* eventSource)
{
  auto converted =
    reinterpret_cast<const TESEvents::TESSpellCastEvent*>(event);

  if (!converted) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto caster = converted->caster.get();
  auto spellId = converted->spell;
  auto casterId = caster ? caster->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([caster, spellId, casterId] {
    auto obj = JsValue::Object();

    auto casterLocal = RE::TESForm::LookupByID(casterId);
    auto spellLocal = RE::TESForm::LookupByID(spellId);

    if (casterLocal == nullptr || spellLocal == nullptr ||
        spellLocal->formType != RE::FormType::Spell || casterLocal != caster) {
      return;
    }

    obj.SetProperty("caster", CreateObject("ObjectReference", casterLocal));
    obj.SetProperty("spell", CreateObject("Spell", spellLocal));

    EventsApi::SendEvent("spellCast", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}
