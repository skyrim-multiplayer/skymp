#include "EventSinks.h"
#include "EventsApi.h"
#include "JsEngine.h"
#include "NativeValueCasts.h"
#include <RE/ActiveEffect.h>
#include <RE/Actor.h>
#include <RE/EffectSetting.h>
#include <RE/TESObjectCELL.h>

extern TaskQueue g_taskQueue;

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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESActivateEvent* event_,
  RE::BSTEventSource<RE::TESActivateEvent>* eventSource)
{
  auto targetRefr = event_ ? event_->target.get() : nullptr;
  auto casterRefr = event_ ? event_->caster.get() : nullptr;

  auto targetId = targetRefr ? targetRefr->formID : 0;
  auto casterId = casterRefr ? casterRefr->formID : 0;

  g_taskQueue.AddTask([targetId, casterId, targetRefr, casterRefr] {
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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESMoveAttachDetachEvent* event_,
  RE::BSTEventSource<RE::TESMoveAttachDetachEvent>* eventSource)
{
  auto movedRef = event_ ? event_->movedRef.get() : nullptr;

  auto targetId = movedRef ? movedRef->formID : 0;
  auto isCellAttached = event_ ? event_->isCellAttached : 0;

  g_taskQueue.AddTask([targetId, isCellAttached, movedRef] {
    auto obj = JsValue::Object();

    auto target = RE::TESForm::LookupByID(targetId);
    target = target == movedRef ? target : nullptr;
    obj.SetProperty("movedRef", CreateObject("ObjectReference", target));

    obj.SetProperty("isCellAttached", JsValue::Bool(isCellAttached));
    EventsApi::SendEvent("moveAttachDetach", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESWaitStopEvent* event_,
  RE::BSTEventSource<RE::TESWaitStopEvent>* eventSource)
{
  auto interrupted = event_ ? event_->interrupted : 0;

  g_taskQueue.AddTask([interrupted] {
    auto obj = JsValue::Object();

    obj.SetProperty("isInterrupted", JsValue::Bool(interrupted));

    EventsApi::SendEvent("waitStop", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESObjectLoadedEvent* event_,
  RE::BSTEventSource<RE::TESObjectLoadedEvent>* eventSource)
{
  auto objectId = event_ ? event_->formID : 0;
  auto loaded = event_ ? event_->loaded : 0;

  g_taskQueue.AddTask([objectId, loaded] {
    auto obj = JsValue::Object();

    obj.SetProperty("object",
                    CreateObject("Form", RE::TESForm::LookupByID(objectId)));

    obj.SetProperty("isLoaded", JsValue::Bool(loaded));

    EventsApi::SendEvent("objectLoaded", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESLockChangedEvent* event_,
  RE::BSTEventSource<RE::TESLockChangedEvent>* eventSource)
{
  auto lockedObject = event_ ? event_->lockedObject : nullptr;
  auto lockedObjectId = lockedObject ? lockedObject->formID : 0;

  g_taskQueue.AddTask([lockedObjectId, lockedObject] {
    auto obj = JsValue::Object();

    auto lockedObject = RE::TESForm::LookupByID(lockedObjectId);
    lockedObject = lockedObject == lockedObject ? lockedObject : nullptr;
    obj.SetProperty("lockedObject",
                    CreateObject("ObjectReference", lockedObject));

    EventsApi::SendEvent("lockChanged", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESCellFullyLoadedEvent* event_,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>* eventSource)
{
  auto cell = event_ ? event_->cell : nullptr;
  auto cellId = cell ? cell->formID : 0;

  g_taskQueue.AddTask([cellId, cell] {
    auto obj = JsValue::Object();

    auto cell_ = RE::TESForm::LookupByID(cellId);
    cell_ = cell_ == cell ? cell_ : nullptr;
    obj.SetProperty("cell", CreateObject("Cell", cell_));

    EventsApi::SendEvent("cellFullyLoaded", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESGrabReleaseEvent* event_,
  RE::BSTEventSource<RE::TESGrabReleaseEvent>* eventSource)
{
  auto ref = event_ ? event_->ref.get() : nullptr;
  auto refId = ref ? ref->formID : 0;
  auto grabbed = event_ ? event_->grabbed : 0;

  g_taskQueue.AddTask([refId, grabbed, ref] {
    auto obj = JsValue::Object();

    auto reference = RE::TESForm::LookupByID(refId);
    reference = reference == ref ? reference : nullptr;
    obj.SetProperty("refr", CreateObject("ObjectReference", reference));

    obj.SetProperty("isGrabbed", JsValue::Bool(grabbed));

    EventsApi::SendEvent("grabRelease", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESLoadGameEvent* event_,
  RE::BSTEventSource<RE::TESLoadGameEvent>* eventSource)
{
  g_taskQueue.AddTask(
    [] { EventsApi::SendEvent("loadGame", { JsValue::Undefined() }); });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESSwitchRaceCompleteEvent* event_,
  RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>* eventSource)
{
  auto subject = event_ ? event_->subject.get() : nullptr;
  auto subjectId = subject ? subject->formID : 0;

  g_taskQueue.AddTask([subjectId, subject] {
    auto obj = JsValue::Object();

    auto subjectLocal = RE::TESForm::LookupByID(subjectId);
    subjectLocal = subjectLocal == subject ? subjectLocal : nullptr;
    obj.SetProperty("subject", CreateObject("ObjectReference", subjectLocal));

    EventsApi::SendEvent("switchRaceComplete", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESUniqueIDChangeEvent* event_,
  RE::BSTEventSource<RE::TESUniqueIDChangeEvent>* eventSource)
{
  auto oldUniqueID = event_ ? event_->oldUniqueID : 0;
  auto newUniqueID = event_ ? event_->newUniqueID : 0;

  auto oldBaseID = event_ ? event_->oldBaseID : 0;
  auto newBaseID = event_ ? event_->newBaseID : 0;

  g_taskQueue.AddTask([oldUniqueID, newUniqueID, oldBaseID, newBaseID] {
    auto obj = JsValue::Object();

    obj.SetProperty("oldBaseID", JsValue::Double(oldBaseID));
    obj.SetProperty("newBaseID", JsValue::Double(newBaseID));
    obj.SetProperty("oldUniqueID", JsValue::Double(oldUniqueID));
    obj.SetProperty("newUniqueID", JsValue::Double(newUniqueID));

    EventsApi::SendEvent("uniqueIdChange", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESTrackedStatsEvent* event_,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>* eventSource)
{
  std::string statName = event_ ? event_->stat.data() : "";
  auto value = event_ ? event_->value : 0;

  g_taskQueue.AddTask([statName, value] {
    auto obj = JsValue::Object();

    obj.SetProperty("statName", JsValue::String(statName));
    obj.SetProperty("newValue", JsValue::Double(value));

    EventsApi::SendEvent("trackedStats", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESInitScriptEvent* event_,
  RE::BSTEventSource<RE::TESInitScriptEvent>* eventSource)
{
  auto objectInitialized = event_ ? event_->objectInitialized.get() : nullptr;
  auto objectInitializedId = objectInitialized ? objectInitialized->formID : 0;

  g_taskQueue.AddTask([objectInitializedId, objectInitialized] {
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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESResetEvent* event_,
  RE::BSTEventSource<RE::TESResetEvent>* eventSource)
{
  auto object = event_ ? event_->object.get() : nullptr;
  auto objectId = object ? object->formID : 0;

  g_taskQueue.AddTask([objectId, object] {
    auto obj = JsValue::Object();

    auto objectIdLocal = RE::TESForm::LookupByID(objectId);
    objectIdLocal = objectIdLocal == object ? objectIdLocal : nullptr;

    obj.SetProperty("object", CreateObject("ObjectReference", objectIdLocal));

    EventsApi::SendEvent("reset", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kStop;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESCombatEvent* event_,
  RE::BSTEventSource<RE::TESCombatEvent>* eventSource)
{
  auto targetActorRefr = event_ ? event_->targetActor.get() : nullptr;
  auto targetActorId = targetActorRefr ? targetActorRefr->formID : 0;

  auto actorRefr = event_ ? event_->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto state = event_ ? (uint32_t)event_->state : 0;

  g_taskQueue.AddTask(
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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESDeathEvent* event_,
  RE::BSTEventSource<RE::TESDeathEvent>* eventSource)
{
  auto actorDyingRefr = event_ ? event_->actorDying.get() : nullptr;
  auto actorDyingId = actorDyingRefr ? actorDyingRefr->formID : 0;

  auto actorKillerRefr = event_ ? event_->actorKiller.get() : nullptr;
  auto actorKillerId = actorKillerRefr ? actorKillerRefr->formID : 0;

  auto dead = event_ ? event_->dead : 0;

  g_taskQueue.AddTask(
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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESContainerChangedEvent* event_,
  RE::BSTEventSource<RE::TESContainerChangedEvent>* eventSource)
{
  auto oldContainerId = event_ ? event_->oldContainer : 0;
  auto newContainerId = event_ ? event_->newContainer : 0;
  auto baseObjId = event_ ? event_->baseObj : 0;
  auto itemCount = event_ ? event_->itemCount : 0;
  auto uniqueID = event_ ? event_->uniqueID : 0;

  auto reference = event_ ? event_->reference.get() : nullptr;
  auto referenceId = reference ? reference->formID : 0;

  g_taskQueue.AddTask([oldContainerId, newContainerId, baseObjId, itemCount,
                       uniqueID, referenceId] {
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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESHitEvent* event_,
  RE::BSTEventSource<RE::TESHitEvent>* eventSource)
{
  auto targetRefr = event_ ? event_->target.get() : nullptr;
  auto causeRefr = event_ ? event_->cause.get() : nullptr;

  auto targetId = targetRefr ? targetRefr->formID : 0;
  auto causeId = causeRefr ? causeRefr->formID : 0;

  auto sourceId = event_ ? event_->source : 0;
  auto projectileId = event_ ? event_->projectile : 0;
  uint8_t flags = event_ ? (uint8_t)event_->flags : 0;

  g_taskQueue.AddTask(
    [targetId, causeId, sourceId, projectileId, flags, targetRefr, causeRefr] {
      auto obj = JsValue::Object();

      auto targetLocal = RE::TESForm::LookupByID(targetId);
      targetLocal = targetLocal == targetRefr ? targetLocal : nullptr;
      obj.SetProperty("target", CreateObject("ObjectReference", targetLocal));

      auto causeLocal = RE::TESForm::LookupByID(causeId);
      causeLocal = causeLocal == causeRefr ? causeLocal : nullptr;
      obj.SetProperty("agressor", CreateObject("ObjectReference", causeLocal));

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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESEquipEvent* event_,
  RE::BSTEventSource<RE::TESEquipEvent>* eventSource)
{
  auto actorRefr = event_ ? event_->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto originalRefrId = event_ ? event_->originalRefr : 0;
  auto baseObjectId = event_ ? event_->baseObject : 0;
  auto equipped = event_ ? event_->equipped : 0;
  auto uniqueId = event_ ? event_->uniqueID : 0;

  g_taskQueue.AddTask([actorId, baseObjectId, equipped, uniqueId,
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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESActiveEffectApplyRemoveEvent* event_,
  RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>* eventSource)
{
  auto caster = event_->caster.get() ? event_->caster.get() : nullptr;
  auto target = event_->target.get() ? event_->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  auto isApplied = event_ ? event_->isApplied : 0;
  auto activeEffectUniqueID = event_ ? event_->activeEffectUniqueID : 0;
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

  g_taskQueue.AddTask([casterId, targetId, isApplied, activeEffectBaseId,
                       caster, target] {
    auto obj = JsValue::Object();

    obj.SetProperty("effect",
                    CreateObject("MagicEffect",
                                 RE::TESForm::LookupByID(activeEffectBaseId)));

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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESMagicEffectApplyEvent* event_,
  RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* eventSource)
{
  auto effectId = event_ ? event_->magicEffect : 0;

  auto caster = event_->caster.get() ? event_->caster.get() : nullptr;
  auto target = event_->target.get() ? event_->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  g_taskQueue.AddTask([effectId, casterId, targetId, caster, target] {
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