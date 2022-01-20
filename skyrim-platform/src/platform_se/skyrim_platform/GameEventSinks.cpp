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
  const RE::TESWaitStartEvent* event,
  RE::BSTEventSource<RE::TESWaitStartEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESWaitStartEvent*>(event);

  auto waitStart = converted->waitStartTime;
  auto waitEnd = converted->desiredWaitEndTime;

  SkyrimPlatform::GetSingleton().AddUpdateTask([waitStart, waitEnd] {
    auto obj = JsValue::Object();

    obj.SetProperty("startTime", JsValue::Double(waitStart));
    obj.SetProperty("desiredStopTime", JsValue::Double(waitEnd));

    EventsApi::SendEvent("waitStart", { JsValue::Undefined(), obj });
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
  const RE::TESCellAttachDetachEvent* event,
  RE::BSTEventSource<RE::TESCellAttachDetachEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESCellAttachDetachEvent*>(event);

  auto reference = converted->reference.get();
  auto action = converted->action;
  auto referenceId = reference ? reference->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [reference, action, referenceId] {
      auto obj = JsValue::Object();

      auto referenceLocal = RE::TESForm::LookupByID(referenceId);

      if (referenceLocal == nullptr || referenceLocal != reference ||
          action < 0 || action > 1) {
        return;
      }

      obj.SetProperty("refr", CreateObject("ObjectReference", reference));

      action > 0
        ? EventsApi::SendEvent("cellAttach", { JsValue::Undefined(), obj })
        : EventsApi::SendEvent("cellDetach", { JsValue::Undefined(), obj });
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
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESSpellCastEvent*>(event);

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

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESOpenCloseEvent* event,
  RE::BSTEventSource<RE::TESOpenCloseEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESOpenCloseEvent*>(event);

  auto target = converted->target.get();
  auto cause = converted->cause.get();
  auto isOpened = converted->isOpened;
  auto targetId = target ? target->formID : 0;
  auto causeId = cause ? cause->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [target, cause, isOpened, targetId, causeId] {
      auto obj = JsValue::Object();

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      auto causeLocal = RE::TESForm::LookupByID(causeId);

      if (targetLocal == nullptr || causeLocal == nullptr ||
          targetLocal != target || causeLocal != cause) {
        return;
      }

      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));
      obj.SetProperty("cause", CreateObject("ObjectReference", causeLocal));

      isOpened ? EventsApi::SendEvent("open", { JsValue::Undefined(), obj })
               : EventsApi::SendEvent("close", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESQuestInitEvent* event,
  RE::BSTEventSource<RE::TESQuestInitEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESQuestInitEvent*>(event);

  auto questId = converted->questId;

  SkyrimPlatform::GetSingleton().AddUpdateTask([questId] {
    auto obj = JsValue::Object();

    auto questLocal = RE::TESForm::LookupByID(questId);

    if (questLocal == nullptr || questLocal->formType != RE::FormType::Quest) {
      return;
    }

    obj.SetProperty("quest", CreateObject("Quest", questLocal));

    EventsApi::SendEvent("questInit", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESQuestStartStopEvent* event,
  RE::BSTEventSource<RE::TESQuestStartStopEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESQuestStartStopEvent*>(event);

  auto questId = converted->questId;
  auto isStarted = converted->isStarted;

  SkyrimPlatform::GetSingleton().AddUpdateTask([questId, isStarted] {
    auto obj = JsValue::Object();

    auto questLocal = RE::TESForm::LookupByID(questId);

    if (questLocal == nullptr || questLocal->formType != RE::FormType::Quest) {
      return;
    }

    obj.SetProperty("quest", CreateObject("Quest", questLocal));

    isStarted
      ? EventsApi::SendEvent("questStart", { JsValue::Undefined(), obj })
      : EventsApi::SendEvent("questStop", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESQuestStageEvent* event,
  RE::BSTEventSource<RE::TESQuestStageEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESQuestStageEvent*>(event);

  auto questId = converted->questId;
  auto stage = converted->stage;

  SkyrimPlatform::GetSingleton().AddUpdateTask([questId, stage] {
    auto obj = JsValue::Object();

    auto questLocal = RE::TESForm::LookupByID(questId);

    if (questLocal == nullptr || questLocal->formType != RE::FormType::Quest) {
      return;
    }

    obj.SetProperty("quest", CreateObject("Quest", questLocal));
    obj.SetProperty("stage", JsValue::Double(stage));

    EventsApi::SendEvent("questStage", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESTriggerEvent* event,
  RE::BSTEventSource<RE::TESTriggerEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted = reinterpret_cast<const TESEvents::TESTriggerEvent*>(event);

  auto target = converted->target.get();
  auto cause = converted->cause.get();
  auto targetId = target ? target->formID : 0;
  auto causeId = cause ? cause->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [target, cause, targetId, causeId] {
      auto obj = JsValue::Object();

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      auto causeLocal = RE::TESForm::LookupByID(causeId);

      if (targetLocal == nullptr || causeLocal == nullptr ||
          targetLocal != target || causeLocal != cause) {
        return;
      }

      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));
      obj.SetProperty("cause", CreateObject("ObjectReference", causeLocal));

      EventsApi::SendEvent("trigger", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESTriggerEnterEvent* event,
  RE::BSTEventSource<RE::TESTriggerEnterEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESTriggerEnterEvent*>(event);

  auto target = converted->target.get();
  auto cause = converted->cause.get();
  auto targetId = target ? target->formID : 0;
  auto causeId = cause ? cause->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [target, cause, targetId, causeId] {
      auto obj = JsValue::Object();

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      auto causeLocal = RE::TESForm::LookupByID(causeId);

      if (targetLocal == nullptr || causeLocal == nullptr ||
          targetLocal != target || causeLocal != cause) {
        return;
      }

      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));
      obj.SetProperty("cause", CreateObject("ObjectReference", causeLocal));

      EventsApi::SendEvent("triggerEnter", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESTriggerLeaveEvent* event,
  RE::BSTEventSource<RE::TESTriggerLeaveEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESTriggerLeaveEvent*>(event);

  auto target = converted->target.get();
  auto cause = converted->cause.get();
  auto targetId = target ? target->formID : 0;
  auto causeId = cause ? cause->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [target, cause, targetId, causeId] {
      auto obj = JsValue::Object();

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      auto causeLocal = RE::TESForm::LookupByID(causeId);

      if (targetLocal == nullptr || causeLocal == nullptr ||
          targetLocal != target || causeLocal != cause) {
        return;
      }

      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));
      obj.SetProperty("cause", CreateObject("ObjectReference", causeLocal));

      EventsApi::SendEvent("triggerLeave", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESSleepStartEvent* event,
  RE::BSTEventSource<RE::TESSleepStartEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESSleepStartEvent*>(event);

  auto sleepStart = converted->sleepStartTime;
  auto sleepEnd = converted->desiredSleepEndTime;

  SkyrimPlatform::GetSingleton().AddUpdateTask([sleepStart, sleepEnd] {
    auto obj = JsValue::Object();

    obj.SetProperty("startTime", JsValue::Double(sleepStart));
    obj.SetProperty("desiredStopTime", JsValue::Double(sleepEnd));

    EventsApi::SendEvent("sleepStart", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESSleepStopEvent* event,
  RE::BSTEventSource<RE::TESSleepStopEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESSleepStopEvent*>(event);

  auto isInterrupted = converted->isInterrupted;

  SkyrimPlatform::GetSingleton().AddUpdateTask([isInterrupted] {
    auto obj = JsValue::Object();

    obj.SetProperty("isInterrupted", JsValue::Bool(isInterrupted));

    EventsApi::SendEvent("sleepStop", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESActorLocationChangeEvent* event,
  RE::BSTEventSource<RE::TESActorLocationChangeEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESActorLocationChangeEvent*>(event);

  auto actor = converted->actor.get();
  auto oldLoc = converted->oldLoc;
  auto newLoc = converted->newLoc;
  auto actorId = actor ? actor->formID : 0;
  auto oldLocId = oldLoc ? oldLoc->formID : 0;
  auto newLocId = newLoc ? newLoc->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [actor, actorId, oldLoc, oldLocId, newLoc, newLocId] {
      auto obj = JsValue::Object();

      auto actorLocal = RE::TESForm::LookupByID(actorId);
      auto oldLocLocal = RE::TESForm::LookupByID(oldLocId);
      auto newLocLocal = RE::TESForm::LookupByID(newLocId);

      if (actorLocal == nullptr || oldLocLocal == nullptr ||
          newLocLocal == nullptr || actorLocal != actor ||
          oldLocLocal != oldLoc || newLocLocal != newLoc) {
        return;
      }

      obj.SetProperty("actor", CreateObject("Actor", actorLocal));
      obj.SetProperty("oldLoc", CreateObject("Location", oldLocLocal));
      obj.SetProperty("newLoc", CreateObject("Location", newLocLocal));

      EventsApi::SendEvent("locationChanged", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESBookReadEvent* event,
  RE::BSTEventSource<RE::TESBookReadEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted = reinterpret_cast<const TESEvents::TESBookReadEvent*>(event);

  auto book = converted->book.get();
  auto bookId = book ? book->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([book, bookId] {
    auto obj = JsValue::Object();

    auto bookLocal = RE::TESForm::LookupByID(bookId);

    if (bookLocal == nullptr || bookLocal != book) {
      return;
    }

    obj.SetProperty("book", CreateObject("ObjectReference", bookLocal));

    EventsApi::SendEvent("bookRead", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESSellEvent* event,
  RE::BSTEventSource<RE::TESSellEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted = reinterpret_cast<const TESEvents::TESSellEvent*>(event);

  auto target = converted->target.get();
  auto seller = converted->seller.get();
  auto targetId = target ? target->formID : 0;
  auto sellerId = seller ? seller->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [target, seller, targetId, sellerId] {
      auto obj = JsValue::Object();

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      auto sellerLocal = RE::TESForm::LookupByID(sellerId);

      if (targetLocal == nullptr || sellerLocal == nullptr ||
          targetLocal != target || sellerLocal != seller) {
        return;
      }

      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));
      obj.SetProperty("seller", CreateObject("ObjectReference", sellerLocal));

      EventsApi::SendEvent("sell", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESFurnitureEvent* event,
  RE::BSTEventSource<RE::TESFurnitureEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESFurnitureEvent*>(event);

  auto target = converted->target.get();
  auto actor = converted->actor.get();
  auto targetId = target ? target->formID : 0;
  auto actorId = actor ? actor->formID : 0;
  auto type = converted->type;

  SkyrimPlatform::GetSingleton().AddUpdateTask([target, actor, targetId,
                                                actorId, type] {
    auto obj = JsValue::Object();

    auto targetLocal = RE::TESForm::LookupByID(targetId);
    auto actorLocal = RE::TESForm::LookupByID(actorId);

    if (targetLocal == nullptr || actorLocal == nullptr ||
        targetLocal != target || actorLocal != actor ||
        static_cast<int>(type) < 0 || static_cast<int>(type) > 1) {
      return;
    }

    obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));
    obj.SetProperty("actor", CreateObject("ObjectReference", actorLocal));

    static_cast<int>(type) == 1
      ? EventsApi::SendEvent("furnitureExit", { JsValue::Undefined(), obj })
      : EventsApi::SendEvent("furnitureEnter", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESMagicWardHitEvent* event,
  RE::BSTEventSource<RE::TESMagicWardHitEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESMagicWardHitEvent*>(event);

  auto target = converted->target.get();
  auto caster = converted->caster.get();
  auto targetId = target ? target->formID : 0;
  auto casterId = caster ? caster->formID : 0;
  auto spellId = converted->spell;
  auto status = converted->status;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [target, caster, targetId, casterId, spellId, status] {
      auto obj = JsValue::Object();

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      auto casterLocal = RE::TESForm::LookupByID(casterId);
      auto spellLocal = RE::TESForm::LookupByID(spellId);

      if (targetLocal == nullptr || casterLocal == nullptr ||
          spellLocal == nullptr || targetLocal != target ||
          casterLocal != caster || static_cast<int>(status) < 0 ||
          static_cast<int>(status) > 2) {
        return;
      }

      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));
      obj.SetProperty("caster", CreateObject("ObjectReference", casterLocal));
      obj.SetProperty("spell", CreateObject("Spell", spellLocal));

      switch (status) {
        case TESEvents::TESMagicWardHitEvent::Status::kFriendly: {
          obj.SetProperty("status", "friendly");
          break;
        }
        case TESEvents::TESMagicWardHitEvent::Status::kAbsorbed: {
          obj.SetProperty("status", "absorbed");
          break;
        }
        case TESEvents::TESMagicWardHitEvent::Status::kBroken: {
          obj.SetProperty("status", "broken");
          break;
        }
      }

      EventsApi::SendEvent("wardHit", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESPackageEvent* event,
  RE::BSTEventSource<RE::TESPackageEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted = reinterpret_cast<const TESEvents::TESPackageEvent*>(event);

  auto actor = converted->actor.get();
  auto actorId = actor ? actor->formID : 0;
  auto packageId = converted->package;
  auto type = converted->type;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [actor, actorId, packageId, type] {
      auto obj = JsValue::Object();

      auto actorLocal = RE::TESForm::LookupByID(actorId);
      auto packageLocal = RE::TESForm::LookupByID(packageId);

      if (actorLocal == nullptr || packageLocal == nullptr ||
          actorLocal != actor || static_cast<int>(type) < 0 ||
          static_cast<int>(type) > 2) {
        return;
      }

      obj.SetProperty("actor", CreateObject("ObjectReference", actorLocal));
      obj.SetProperty("package", CreateObject("Package", packageLocal));

      switch (type) {
        case TESEvents::TESPackageEvent::EventType::kStart: {
          EventsApi::SendEvent("packageStart", { JsValue::Undefined(), obj });
          break;
        }
        case TESEvents::TESPackageEvent::EventType::kChange: {
          EventsApi::SendEvent("packageChange", { JsValue::Undefined(), obj });
          break;
        }
        case TESEvents::TESPackageEvent::EventType::kEnd: {
          EventsApi::SendEvent("packageEnd", { JsValue::Undefined(), obj });
          break;
        }
      }
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESEnterBleedoutEvent* event,
  RE::BSTEventSource<RE::TESEnterBleedoutEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESEnterBleedoutEvent*>(event);

  auto actor = converted->actor.get();
  auto actorId = actor ? actor->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([actor, actorId] {
    auto obj = JsValue::Object();

    auto actorLocal = RE::TESForm::LookupByID(actorId);

    if (actorLocal == nullptr || actorLocal != actor) {
      return;
    }

    obj.SetProperty("actor", CreateObject("ObjectReference", actorLocal));

    EventsApi::SendEvent("enterBleedout", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const SKSE::ActionEvent* event,
  RE::BSTEventSource<SKSE::ActionEvent>* eventSource)
{

  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto type = event->type;
  auto actor = event->actor;
  auto actorId = actor ? actor->formID : 0;
  auto source = event->sourceForm;
  auto sourceId = source ? source->formID : 0;
  auto slot = event->slot;

  SkyrimPlatform::GetSingleton().AddUpdateTask([type, actor, actorId, source,
                                                sourceId, slot] {
    auto obj = JsValue::Object();

    auto actorLocal = RE::TESForm::LookupByID(actorId);
    auto sourceLocal = RE::TESForm::LookupByID(sourceId);

    if (actorLocal == nullptr || sourceLocal == nullptr ||
        actorLocal != actor || sourceLocal != source) {
      return;
    }

    obj.SetProperty("actor", CreateObject("Actor", actorLocal));
    obj.SetProperty("source", CreateObject("Form", sourceLocal));
    obj.SetProperty("slot", JsValue::Double(static_cast<double>(slot)));

    switch (type) {
      case SKSE::ActionEvent::Type::kWeaponSwing: {
        EventsApi::SendEvent("actionWeaponSwing",
                             { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kBeginDraw: {
        EventsApi::SendEvent("actionBeginDraw", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kEndDraw: {
        EventsApi::SendEvent("actionEndDraw", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kBowDraw: {
        EventsApi::SendEvent("actionBowDraw", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kBowRelease: {
        EventsApi::SendEvent("actionBowRelease",
                             { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kBeginSheathe: {
        EventsApi::SendEvent("actionBeginSheathe",
                             { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kEndSheathe: {
        EventsApi::SendEvent("actionEndSheathe",
                             { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kSpellCast: {
        EventsApi::SendEvent("actionSpellCast", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kSpellFire: {
        EventsApi::SendEvent("actionSpellFire", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kVoiceCast: {
        EventsApi::SendEvent("actionVoiceCast", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kVoiceFire: {
        EventsApi::SendEvent("actionVoiceFire", { JsValue::Undefined(), obj });
        break;
      }
    }
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const SKSE::CameraEvent* event,
  RE::BSTEventSource<SKSE::CameraEvent>* eventSource)
{

  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto newState = event->newState;
  auto oldState = event->oldState;

  if (newState == nullptr || oldState == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto oldStateId = oldState->id;
  auto newStateId = newState->id;

  SkyrimPlatform::GetSingleton().AddUpdateTask([oldStateId, newStateId] {
    auto obj = JsValue::Object();

    obj.SetProperty("oldStateId", JsValue::Double(oldStateId));
    obj.SetProperty("newStateId", JsValue::Double(newStateId));

    EventsApi::SendEvent("cameraStateChanged", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const SKSE::CrosshairRefEvent* event,
  RE::BSTEventSource<SKSE::CrosshairRefEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto ref = event->crosshairRef.get();
  auto refId = ref ? ref->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([ref, refId] {
    auto obj = JsValue::Object();

    auto refLocal = RE::TESForm::LookupByID(refId);

    if (refId == 0) {
      obj.SetProperty("reference", JsValue::Null());
    } else {
      if (refLocal == nullptr || refLocal != ref) {
        return;
      }
      obj.SetProperty("reference", CreateObject("ObjectReference", refLocal));
    }

    EventsApi::SendEvent("crosshairRefChanged", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const SKSE::NiNodeUpdateEvent* event,
  RE::BSTEventSource<SKSE::NiNodeUpdateEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto ref = event->reference;
  auto refId = ref ? ref->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([ref, refId] {
    auto obj = JsValue::Object();

    auto refLocal = RE::TESForm::LookupByID(refId);

    if (refId == 0) {
      obj.SetProperty("reference", JsValue::Null());
    } else {
      if (refLocal == nullptr || refLocal != ref) {
        return;
      }
      obj.SetProperty("reference", CreateObject("ObjectReference", refLocal));
    }

    EventsApi::SendEvent("niNodeUpdate", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const SKSE::ModCallbackEvent* event,
  RE::BSTEventSource<SKSE::ModCallbackEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto eventName = static_cast<std::string>(event->eventName);
  auto numArg = event->numArg;
  auto strArg = static_cast<std::string>(event->strArg);
  auto sender = event->sender;
  auto senderId = sender ? sender->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [eventName, numArg, strArg, sender, senderId] {
      auto obj = JsValue::Object();

      auto senderLocal = RE::TESForm::LookupByID(senderId);

      if (senderLocal == nullptr || senderLocal != sender) {
        return;
      }

      obj.SetProperty("sender", CreateObject("Form", senderLocal));
      obj.SetProperty("eventName", JsValue::String(eventName));
      obj.SetProperty("strArg", JsValue::String(strArg));
      obj.SetProperty("numArg", JsValue::Double(numArg));

      EventsApi::SendEvent("modEvent", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESDestructionStageChangedEvent* event,
  RE::BSTEventSource<RE::TESDestructionStageChangedEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESDestructionStageChangedEvent*>(event);

  auto target = converted->target.get();
  auto targetId = target ? target->formID : 0;
  auto oldStage = converted->oldStage;
  auto newStage = converted->newStage;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [target, targetId, oldStage, newStage] {
      auto obj = JsValue::Object();

      auto targetLocal = RE::TESForm::LookupByID(targetId);

      if (targetLocal == nullptr || targetLocal != target) {
        return;
      }

      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));
      obj.SetProperty("oldStage", JsValue::Double(oldStage));
      obj.SetProperty("newStage", JsValue::Double(newStage));

      EventsApi::SendEvent("destructionStageChanged",
                           { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESSceneActionEvent* event,
  RE::BSTEventSource<RE::TESSceneActionEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESSceneActionEvent*>(event);

  auto sceneId = converted->sceneId;
  auto refAliasId = converted->referenceAliasID;
  auto questId = converted->questId;
  auto action = converted->action;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [sceneId, refAliasId, questId, action] {
      auto obj = JsValue::Object();

      auto sceneLocal = RE::TESForm::LookupByID(sceneId);
      auto questLocal = RE::TESForm::LookupByID(questId);

      if (sceneLocal == nullptr || questLocal == nullptr || refAliasId < 0 ||
          action < 0 || sceneLocal->formType != RE::FormType::Scene ||
          questLocal->formType != RE::FormType::Quest) {
        return;
      }

      obj.SetProperty("referenceAliasId", JsValue::Double(refAliasId));
      obj.SetProperty("scene", CreateObject("Scene", sceneLocal));
      obj.SetProperty("quest", CreateObject("Quest", questLocal));
      obj.SetProperty("action", JsValue::Double(action));

      EventsApi::SendEvent("sceneAction", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESPlayerBowShotEvent* event,
  RE::BSTEventSource<RE::TESPlayerBowShotEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESPlayerBowShotEvent*>(event);

  auto weaponId = converted->weaponId;
  auto ammoId = converted->ammoId;
  auto power = converted->power;
  auto isSunGazing = converted->isSunGazing;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [weaponId, ammoId, power, isSunGazing] {
      auto obj = JsValue::Object();

      auto weaponLocal = RE::TESForm::LookupByID(weaponId);
      auto ammoLocal = RE::TESForm::LookupByID(ammoId);

      if (weaponLocal == nullptr || ammoLocal == nullptr ||
          weaponLocal->formType != RE::FormType::Weapon ||
          ammoLocal->formType != RE::FormType::Ammo) {
        return;
      }

      obj.SetProperty("weapon", CreateObject("Weapon", weaponLocal));
      obj.SetProperty("ammo", CreateObject("Ammo", ammoLocal));
      obj.SetProperty("power", JsValue::Double(power));
      obj.SetProperty("isSunGazing", JsValue::Bool(isSunGazing));

      EventsApi::SendEvent("playerBowShot", { JsValue::Undefined(), obj });
    });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESFastTravelEndEvent* event,
  RE::BSTEventSource<RE::TESFastTravelEndEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESFastTravelEndEvent*>(event);

  auto travelTimeGameHours = converted->travelTimeGameHours;

  SkyrimPlatform::GetSingleton().AddUpdateTask([travelTimeGameHours] {
    auto obj = JsValue::Object();

    obj.SetProperty("travelTimeGameHours",
                    JsValue::Double(travelTimeGameHours));

    EventsApi::SendEvent("fastTravelEnd", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESObjectREFRTranslationEvent* event,
  RE::BSTEventSource<RE::TESObjectREFRTranslationEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }
  auto converted =
    reinterpret_cast<const TESEvents::TESObjectREFRTranslationEvent*>(event);

  auto ref = converted->refr.get();
  auto refId = ref ? ref->formID : 0;
  auto type = converted->type;

  SkyrimPlatform::GetSingleton().AddUpdateTask([ref, refId, type] {
    auto obj = JsValue::Object();

    auto refLocal = RE::TESForm::LookupByID(refId);

    if (refLocal == nullptr || refLocal != ref) {
      return;
    }

    obj.SetProperty("reference", CreateObject("ObjectReference", refLocal));

    switch (type) {
      case TESEvents::TESObjectREFRTranslationEvent::EventType::kFailed: {
        EventsApi::SendEvent("translationFailed",
                             { JsValue::Undefined(), obj });
        break;
      }
      case TESEvents::TESObjectREFRTranslationEvent::EventType::
        kAlmostCompleted: {
        EventsApi::SendEvent("translationAlmostCompleted",
                             { JsValue::Undefined(), obj });
        break;
      }
      case TESEvents::TESObjectREFRTranslationEvent::EventType::kCompleted: {
        EventsApi::SendEvent("translationCompleted",
                             { JsValue::Undefined(), obj });
        break;
      }
    }
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::TESPerkEntryRunEvent* event,
  RE::BSTEventSource<RE::TESPerkEntryRunEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto converted =
    reinterpret_cast<const TESEvents::TESPerkEntryRunEvent*>(event);

  auto cause = converted->cause.get();
  auto causeId = cause ? cause->formID : 0;
  auto target = converted->target.get();
  auto targetId = target ? target->formID : 0;
  auto perkId = converted->perkId;
  auto flag = converted->flag;

  SkyrimPlatform::GetSingleton().AddUpdateTask([cause, causeId, target,
                                                targetId, perkId, flag] {
    auto obj = JsValue::Object();

    auto causeLocal = RE::TESForm::LookupByID(causeId);
    auto targetLocal = RE::TESForm::LookupByID(targetId);
    auto perkLocal = RE::TESForm::LookupByID(perkId);

    if (causeLocal == nullptr || targetLocal == nullptr ||
        perkLocal == nullptr || causeLocal != cause || targetLocal != target ||
        perkLocal->formType != RE::FormType::Perk || flag < 0) {
      return;
    }

    obj.SetProperty("cause", CreateObject("ObjectReference", causeLocal));
    obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));
    obj.SetProperty("perk", CreateObject("Perk", perkLocal));
    obj.SetProperty("flag", JsValue::Double(flag));

    EventsApi::SendEvent("perkEntryRun", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::PositionPlayerEvent* event,
  RE::BSTEventSource<RE::PositionPlayerEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto type = event->type;

  SkyrimPlatform::GetSingleton().AddUpdateTask([type] {
    auto obj = JsValue::Object();

    obj.SetProperty("eventType", JsValue::Double(static_cast<int>(type)));

    EventsApi::SendEvent("positionPlayer", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GameEventSinks::ProcessEvent(
  const RE::BGSFootstepEvent* event,
  RE::BSTEventSource<RE::BGSFootstepEvent>* eventSource)
{
  if (event == nullptr) {
    return RE::BSEventNotifyControl::kContinue;
  }

  auto tag = static_cast<std::string>(event->tag);

  SkyrimPlatform::GetSingleton().AddUpdateTask([tag] {
    auto obj = JsValue::Object();

    obj.SetProperty("tag", JsValue::String(tag));

    EventsApi::SendEvent("footstep", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}
