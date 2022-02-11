#include "EventHandler.h"
#include "EventsApi.h"
#include "JsUtils.h"
#include "SkyrimPlatform.h"

namespace {
inline void SendEvent(const char* name, JsValue obj)
{
  EventsApi::SendEvent(name, { JsValue::Undefined(), obj });
}
}

EventResult EventHandler::ProcessEvent(
  const RE::TESActivateEvent* event, RE::BSTEventSource<RE::TESActivateEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyPtr(event);

  SkyrimPlatform::GetSingleton().AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "target", e->objectActivated.get(),
                   "ObjectReference");
    AddObjProperty(&obj, "caster", e->actionRef.get(), "ObjectReference");
    AddObjProperty(&obj, "isCrimeToActivate",
                   e->objectActivated.get()->IsCrimeToActivate());

    SendEvent("activate", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESActiveEffectApplyRemoveEvent* event,
  RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>*)
{
  auto caster = event->caster.get() ? event->caster.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  if ((!caster || caster->formID != casterId) ||
      (!target || target->formID != targetId) ||
      (target->formType.get() != RE::FormType::ActorCharacter)) {
    return EventResult::kContinue;
  }

  auto activeEffectUniqueID = event ? event->activeEffectUniqueID : 0;

  RE::ActiveEffect* activeEffect = nullptr;
  for (const auto& effect : *target->As<RE::Actor>()->GetActiveEffectList()) {
    if (effect->usUniqueID == event->activeEffectUniqueID) {
      activeEffect = effect;
      break;
    }
  }

  if (!activeEffect)
    return EventResult::kContinue;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "effect", activeEffect->GetBaseObject(),
                   "MagicEffect");
    AddObjProperty(&obj, "activeEffect", activeEffect, "ActiveMagicEffect");
    AddObjProperty(&obj, "caster", caster, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");

    if (event->isApplied)
      EventsApi::SendEvent("effectStart", { JsValue::Undefined(), obj });
    else
      EventsApi::SendEvent("effectFinish", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESActorLocationChangeEvent* event,
  RE::BSTEventSource<RE::TESActorLocationChangeEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyPtr(event);

  // auto actor = event->actor.get() ? event->actor.get() : nullptr;

  // auto actorId = actor ? actor->formID : 0;

  // if ((!actor || actor->formID != actorId) ||
  //     (event->oldLoc->formType != RE::FormType::Location) ||
  //     (event->newLoc->formType != RE::FormType::Location)) {
  //   return EventResult::kContinue;
  // }

  SkyrimPlatform::GetSingleton().AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "actor", e->actor.get(), "Actor");
    AddObjProperty(&obj, "oldLoc", e->oldLoc, "Location");
    AddObjProperty(&obj, "newLoc", e->newLoc, "Location");

    SendEvent("locationChanged", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESBookReadEvent* event, RE::BSTEventSource<RE::TESBookReadEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyPtr(event);

  SkyrimPlatform::GetSingleton().AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "book", e->book.get(), "ObjectReference");

    SendEvent("bookRead", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESCellAttachDetachEvent* event,
  RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyPtr(event);

  // auto refr = event->reference.get();
  // auto action = event->action;

  // auto formId = refr ? refr->formID : 0;
  // auto form = RE::TESForm::LookupByID(formId);

  // if (!form || refr->formID != form->formID) {
  //   return EventResult::kContinue;
  // }

  SkyrimPlatform::GetSingleton().AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "refr", e->reference.get(), "ObjectReference");

    if (e->action == 1)
      SendEvent("cellAttach", obj);

    if (e->action == 0)
      SendEvent("cellDetach", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESCellFullyLoadedEvent* event,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyPtr(event);

  SkyrimPlatform::GetSingleton().AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "cell", e->cell, "Cell");

    SendEvent("cellFullyLoaded", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESCombatEvent* event,
                                       RE::BSTEventSource<RE::TESCombatEvent>*)
{
  auto targetActorRefr = event ? event->targetActor.get() : nullptr;
  auto targetActorId = targetActorRefr ? targetActorRefr->formID : 0;

  auto actorRefr = event ? event->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto state = event
    ? event->newState
    : RE::stl::enumeration<RE::ACTOR_COMBAT_STATE, std::uint32_t>(
        RE::ACTOR_COMBAT_STATE::kNone);

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    auto targetActorLocal = RE::TESForm::LookupByID(targetActorId);
    targetActorLocal =
      targetActorLocal == targetActorRefr ? targetActorLocal : nullptr;
    AddObjProperty(&obj, "target", targetActorLocal, "ObjectReference");

    auto actorLocal = RE::TESForm::LookupByID(actorId);
    actorLocal = actorLocal == actorRefr ? actorLocal : nullptr;
    AddObjProperty(&obj, "actor", actorLocal, "ObjectReference");

    AddObjProperty(&obj, "isCombat",
                   state.get() == RE::ACTOR_COMBAT_STATE::kCombat);
    AddObjProperty(&obj, "isSearching",
                   state.get() == RE::ACTOR_COMBAT_STATE::kSearching);

    EventsApi::SendEvent("combatState", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESContainerChangedEvent* event,
  RE::BSTEventSource<RE::TESContainerChangedEvent>*)
{
  auto oldContainerId = event ? event->oldContainer : 0;
  auto newContainerId = event ? event->newContainer : 0;
  auto baseObjId = event ? event->baseObj : 0;
  auto itemCount = event ? event->itemCount : 0;
  auto uniqueID = event ? event->uniqueID : 0;

  auto reference = event ? event->reference.get() : nullptr;
  auto referenceId = reference ? reference->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "oldContainer",
                   RE::TESForm::LookupByID(oldContainerId), "ObjectReference");
    AddObjProperty(&obj, "newContainer",
                   RE::TESForm::LookupByID(newContainerId), "ObjectReference");
    AddObjProperty(&obj, "baseObj", RE::TESForm::LookupByID(baseObjId),
                   "Form");
    AddObjProperty(&obj, "numItems", itemCount);
    AddObjProperty(&obj, "uniqueID", uniqueID);
    AddObjProperty(&obj, "reference", RE::TESForm::LookupByID(referenceId),
                   "ObjectReference");

    EventsApi::SendEvent("containerChanged", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESDeathEvent* event,
                                       RE::BSTEventSource<RE::TESDeathEvent>*)
{
  auto actorDyingRefr = event ? event->actorDying.get() : nullptr;
  auto actorDyingId = actorDyingRefr ? actorDyingRefr->formID : 0;

  auto actorKillerRefr = event ? event->actorKiller.get() : nullptr;
  auto actorKillerId = actorKillerRefr ? actorKillerRefr->formID : 0;

  auto dead = event ? event->dead : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    auto actorDyingLocal = RE::TESForm::LookupByID(actorDyingId);
    actorDyingLocal =
      actorDyingLocal == actorDyingRefr ? actorDyingLocal : nullptr;

    AddObjProperty(&obj, "actorDying", actorDyingLocal, "ObjectReference");

    auto actorKillerLocal = RE::TESForm::LookupByID(actorKillerId);
    actorKillerLocal =
      actorKillerLocal == actorKillerRefr ? actorKillerLocal : nullptr;

    AddObjProperty(&obj, "actorKiller", actorKillerLocal, "ObjectReference");

    dead ? EventsApi::SendEvent("deathEnd", { JsValue::Undefined(), obj })
         : EventsApi::SendEvent("deathStart", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESDestructionStageChangedEvent* event,
  RE::BSTEventSource<RE::TESDestructionStageChangedEvent>*)
{
  auto target = event->target ? event->target.get() : nullptr;
  auto targetId = target ? target->formID : 0;

  if (!target || target->formID != targetId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "target", target, "ObjectReference");
    AddObjProperty(&obj, "oldStage", event->oldStage);
    AddObjProperty(&obj, "newStage", event->newStage);

    EventsApi::SendEvent("destructionStageChanged",
                         { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESEnterBleedoutEvent* event,
  RE::BSTEventSource<RE::TESEnterBleedoutEvent>*)
{
  auto actor = event->actor.get() ? event->actor.get() : nullptr;
  auto actorId = actor ? actor->formID : 0;

  if (!actor || actor->formID != actorId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "actor", actor, "ObjectReference");

    EventsApi::SendEvent("enterBleedout", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESEquipEvent* event,
                                       RE::BSTEventSource<RE::TESEquipEvent>*)
{
  auto actorRefr = event ? event->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto originalRefrId = event ? event->originalRefr : 0;
  auto baseObjectId = event ? event->baseObject : 0;
  auto equipped = event ? event->equipped : 0;
  auto uniqueId = event ? event->uniqueID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    auto actorLocal = RE::TESForm::LookupByID(actorId);
    actorLocal = actorLocal == actorRefr ? actorLocal : nullptr;
    AddObjProperty(&obj, "actor", actorLocal, "ObjectReference");

    AddObjProperty(&obj, "baseObj", RE::TESForm::LookupByID(baseObjectId),
                   "Form");
    AddObjProperty(&obj, "originalRefr",
                   RE::TESForm::LookupByID(originalRefrId), "ObjectReference");
    AddObjProperty(&obj, "uniqueId", uniqueId);

    equipped ? EventsApi::SendEvent("equip", { JsValue::Undefined(), obj })
             : EventsApi::SendEvent("unequip", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESFastTravelEndEvent* event,
  RE::BSTEventSource<RE::TESFastTravelEndEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "travelTimeGameHours", event->travelTimeGameHours);

    EventsApi::SendEvent("fastTravelEnd", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESFurnitureEvent* event,
  RE::BSTEventSource<RE::TESFurnitureEvent>*)
{
  auto actor = event->actor.get() ? event->actor.get() : nullptr;
  auto target =
    event->targetFurniture.get() ? event->targetFurniture.get() : nullptr;

  auto actorId = actor ? actor->formID : 0;
  auto targetId = target ? target->formID : 0;

  if ((!actor || actor->formID != actorId) ||
      (!target || target->formID != targetId)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "actor", actor, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");

    if (event->type == RE::TESFurnitureEvent::FurnitureEventType::kExit) {
      EventsApi::SendEvent("furnitureExit", { JsValue::Undefined(), obj });
    } else if (event->type ==
               RE::TESFurnitureEvent::FurnitureEventType::kEnter) {
      EventsApi::SendEvent("furnitureEnter", { JsValue::Undefined(), obj });
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESGrabReleaseEvent* event,
  RE::BSTEventSource<RE::TESGrabReleaseEvent>*)
{
  auto ref = event ? event->ref.get() : nullptr;
  auto refId = ref ? ref->formID : 0;
  auto grabbed = event ? event->grabbed : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    auto reference = RE::TESForm::LookupByID(refId);
    reference = reference == ref ? reference : nullptr;
    AddObjProperty(&obj, "refr", reference, "ObjectReference");

    AddObjProperty(&obj, "isGrabbed", grabbed);

    EventsApi::SendEvent("grabRelease", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESHitEvent* event,
                                       RE::BSTEventSource<RE::TESHitEvent>*)
{
  auto targetRefr = event ? event->target.get() : nullptr;
  auto causeRefr = event ? event->cause.get() : nullptr;

  auto targetId = targetRefr ? targetRefr->formID : 0;
  auto causeId = causeRefr ? causeRefr->formID : 0;

  auto sourceId = event ? event->source : 0;
  auto projectileId = event ? event->projectile : 0;
  auto flags = event ? event->flags
                     : RE::stl::enumeration<RE::TESHitEvent::Flag, uint8_t>(
                         RE::TESHitEvent::Flag::kNone);

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    auto targetLocal = RE::TESForm::LookupByID(targetId);
    targetLocal = targetLocal == targetRefr ? targetLocal : nullptr;
    AddObjProperty(&obj, "target", targetLocal, "ObjectReference");

    auto causeLocal = RE::TESForm::LookupByID(causeId);
    causeLocal = causeLocal == causeRefr ? causeLocal : nullptr;
    // TODO(#336): drop old name "agressor" on next major release of SP
    AddObjProperty(&obj, "agressor", causeLocal, "ObjectReference");

    AddObjProperty(&obj, "aggressor", causeLocal, "ObjectReference");
    AddObjProperty(&obj, "source", RE::TESForm::LookupByID(sourceId), "Form");
    AddObjProperty(&obj, "projectile", RE::TESForm::LookupByID(projectileId),
                   "Form");
    AddObjProperty(&obj, "isPowerAttack",
                   flags.any(RE::TESHitEvent::Flag::kPowerAttack));
    AddObjProperty(&obj, "isSneakAttack",
                   flags.any(RE::TESHitEvent::Flag::kSneakAttack));
    AddObjProperty(&obj, "isBashAttack",
                   flags.any(RE::TESHitEvent::Flag::kBashAttack));
    AddObjProperty(&obj, "isHitBlocked",
                   flags.any(RE::TESHitEvent::Flag::kHitBlocked));

    EventsApi::SendEvent("hit", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESInitScriptEvent* event,
  RE::BSTEventSource<RE::TESInitScriptEvent>*)
{
  auto objectInitialized = event ? event->objectInitialized.get() : nullptr;
  auto objectInitializedId = objectInitialized ? objectInitialized->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    auto objectInitializedLocal = RE::TESForm::LookupByID(objectInitializedId);
    objectInitializedLocal = objectInitializedLocal == objectInitialized
      ? objectInitializedLocal
      : nullptr;
    AddObjProperty(&obj, "initializedObject", objectInitializedLocal,
                   "ObjectReference");

    EventsApi::SendEvent("scriptInit", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

// TODO: Look into LoadGame event
EventResult EventHandler::ProcessEvent(
  const RE::TESLoadGameEvent* event, RE::BSTEventSource<RE::TESLoadGameEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [] { EventsApi::SendEvent("loadGame", { JsValue::Undefined() }); });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESLockChangedEvent* event,
  RE::BSTEventSource<RE::TESLockChangedEvent>*)
{
  auto lockedObject = event ? event->lockedObject : nullptr;
  auto lockedObjectId = lockedObject ? lockedObject->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([lockedObjectId, lockedObject] {
    auto obj = JsValue::Object();

    auto lockedObject = RE::TESForm::LookupByID(lockedObjectId);
    lockedObject = lockedObject == lockedObject ? lockedObject : nullptr;
    AddObjProperty(&obj, "lockedObject", lockedObject, "ObjectReference");

    EventsApi::SendEvent("lockChanged", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESMagicEffectApplyEvent* event,
  RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*)
{
  auto caster = event->caster.get() ? event->caster.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  auto effect = RE::TESForm::LookupByID(event->magicEffect);

  if ((!caster || caster->formID != casterId) ||
      (!target || target->formID != targetId) ||
      (effect->formType != RE::FormType::MagicEffect)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "effect", effect, "MagicEffect");
    AddObjProperty(&obj, "caster", caster, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("magicEffectApply", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESMagicWardHitEvent* event,
  RE::BSTEventSource<RE::TESMagicWardHitEvent>*)
{
  auto caster = event->caster.get() ? event->caster.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  auto spell = RE::TESForm::LookupByID(event->spell);

  if ((!caster || caster->formID != casterId) ||
      (!target || target->formID != targetId) ||
      (spell->formType.get() != RE::FormType::Spell)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "caster", caster, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");
    AddObjProperty(&obj, "spell", spell, "Spell");

    switch (event->status) {
      case RE::TESMagicWardHitEvent::Status::kFriendly: {
        AddObjProperty(&obj, "status", "friendly");
        break;
      }
      case RE::TESMagicWardHitEvent::Status::kAbsorbed: {
        AddObjProperty(&obj, "status", "absorbed");
        break;
      }
      case RE::TESMagicWardHitEvent::Status::kBroken: {
        AddObjProperty(&obj, "status", "broken");
        break;
      }
    }

    EventsApi::SendEvent("wardHit", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESMoveAttachDetachEvent* event,
  RE::BSTEventSource<RE::TESMoveAttachDetachEvent>*)
{
  auto movedRef = event ? event->movedRef.get() : nullptr;

  auto targetId = movedRef ? movedRef->formID : 0;
  auto isCellAttached = event ? event->isCellAttached : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    auto target = RE::TESForm::LookupByID(targetId);
    target = target == movedRef ? target : nullptr;
    AddObjProperty(&obj, "movedRef", target, "ObjectReference");

    AddObjProperty(&obj, "isCellAttached", isCellAttached);

    EventsApi::SendEvent("moveAttachDetach", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESObjectLoadedEvent* event,
  RE::BSTEventSource<RE::TESObjectLoadedEvent>*)
{
  auto objectId = event ? event->formID : 0;
  auto loaded = event ? event->loaded : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "object", RE::TESForm::LookupByID(objectId), "Form");
    AddObjProperty(&obj, "isLoaded", loaded);

    EventsApi::SendEvent("objectLoaded", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESObjectREFRTranslationEvent* event,
  RE::BSTEventSource<RE::TESObjectREFRTranslationEvent>*)
{
  if (event == nullptr) {
    return EventResult::kContinue;
  }
  auto converted =
    reinterpret_cast<const RE::TESObjectREFRTranslationEvent*>(event);

  auto ref = converted->refr.get();
  auto refId = ref ? ref->formID : 0;
  auto type = converted->type;

  SkyrimPlatform::GetSingleton().AddUpdateTask([ref, refId, type] {
    auto obj = JsValue::Object();

    auto refLocal = RE::TESForm::LookupByID(refId);

    if (refLocal == nullptr || refLocal != ref) {
      return;
    }

    AddObjProperty(&obj, "reference", refLocal, "ObjectReference");

    switch (type) {
      case RE::TESObjectREFRTranslationEvent::EventType::kFailed: {
        EventsApi::SendEvent("translationFailed",
                             { JsValue::Undefined(), obj });
        break;
      }
      case RE::TESObjectREFRTranslationEvent::EventType::kAlmostCompleted: {
        EventsApi::SendEvent("translationAlmostCompleted",
                             { JsValue::Undefined(), obj });
        break;
      }
      case RE::TESObjectREFRTranslationEvent::EventType::kCompleted: {
        EventsApi::SendEvent("translationCompleted",
                             { JsValue::Undefined(), obj });
        break;
      }
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESOpenCloseEvent* event,
  RE::BSTEventSource<RE::TESOpenCloseEvent>*)
{
  auto caster = event->activeRef.get() ? event->activeRef.get() : nullptr;
  auto target = event->ref.get() ? event->ref.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  if ((!caster || caster->formID != casterId) ||
      (!target || target->formID != targetId)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "caster", caster, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");

    if (event->opened) {
      EventsApi::SendEvent("open", { JsValue::Undefined(), obj });
    } else {
      EventsApi::SendEvent("close", { JsValue::Undefined(), obj });
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESPackageEvent* event, RE::BSTEventSource<RE::TESPackageEvent>*)
{
  auto actor = event->actor.get() ? event->actor.get() : nullptr;
  auto actorId = actor ? actor->formID : 0;

  auto package = RE::TESForm::LookupByID(event->package);
  if ((!actor || actor->formID != actorId) ||
      (package->formType.get() != RE::FormType::Package)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "actor", actor, "ObjectReference");
    AddObjProperty(&obj, "package", package, "Package");

    switch (event->type) {
      case RE::TESPackageEvent::EventType::kStart: {
        EventsApi::SendEvent("packageStart", { JsValue::Undefined(), obj });
        break;
      }
      case RE::TESPackageEvent::EventType::kChange: {
        EventsApi::SendEvent("packageChange", { JsValue::Undefined(), obj });
        break;
      }
      case RE::TESPackageEvent::EventType::kEnd: {
        EventsApi::SendEvent("packageEnd", { JsValue::Undefined(), obj });
        break;
      }
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESPerkEntryRunEvent* event,
  RE::BSTEventSource<RE::TESPerkEntryRunEvent>*)
{
  auto cause = event->cause ? event->cause.get() : nullptr;
  auto target = event->target ? event->target.get() : nullptr;

  auto causeId = cause ? cause->formID : 0;
  auto targetId = target ? target->formID : 0;

  auto perk = RE::TESForm::LookupByID(event->perkId);

  if ((!cause || cause->formID != causeId) ||
      (!target || target->formID != targetId) ||
      (perk->formType.get() != RE::FormType::Perk)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "cause", cause, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");
    AddObjProperty(&obj, "perk", perk, "Perk");
    AddObjProperty(&obj, "flag", event->flag);

    EventsApi::SendEvent("perkEntryRun", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESPlayerBowShotEvent* event,
  RE::BSTEventSource<RE::TESPlayerBowShotEvent>*)
{
  auto weapon = RE::TESForm::LookupByID(event->weaponId);
  auto ammo = RE::TESForm::LookupByID(event->ammoId);

  if (weapon->formType.get() != RE::FormType::Weapon ||
      ammo->formType.get() != RE::FormType::Ammo) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "weapon", weapon, "Weapon");
    AddObjProperty(&obj, "ammo", ammo, "Ammo");
    AddObjProperty(&obj, "power", event->power);
    AddObjProperty(&obj, "target", event->isSunGazing);

    EventsApi::SendEvent("playerBowShot", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESQuestInitEvent* event,
  RE::BSTEventSource<RE::TESQuestInitEvent>*)
{
  auto quest = RE::TESForm::LookupByID(event->questId);

  if (quest->formType.get() != RE::FormType::Quest)
    return EventResult::kContinue;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "quest", quest, "Quest");

    EventsApi::SendEvent("questInit", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESQuestStageEvent* event,
  RE::BSTEventSource<RE::TESQuestStageEvent>*)
{
  auto quest = RE::TESForm::LookupByID(event->questId);

  if (quest->formType.get() != RE::FormType::Quest)
    return EventResult::kContinue;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "quest", quest, "Quest");
    AddObjProperty(&obj, "stage", event->stage);

    EventsApi::SendEvent("questStage", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESQuestStartStopEvent* event,
  RE::BSTEventSource<RE::TESQuestStartStopEvent>*)
{
  auto quest = RE::TESForm::LookupByID(event->questId);

  if (quest->formType.get() != RE::FormType::Quest)
    return EventResult::kContinue;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "quest", quest, "Quest");

    if (event->isStarted) {
      EventsApi::SendEvent("questStart", { JsValue::Undefined(), obj });
    } else {
      EventsApi::SendEvent("questStop", { JsValue::Undefined(), obj });
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESResetEvent* event,
                                       RE::BSTEventSource<RE::TESResetEvent>*)
{
  auto object = event ? event->object.get() : nullptr;
  auto objectId = object ? object->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    auto objectIdLocal = RE::TESForm::LookupByID(objectId);
    objectIdLocal = objectIdLocal == object ? objectIdLocal : nullptr;
    AddObjProperty(&obj, "object", objectIdLocal, "ObjectReference");

    EventsApi::SendEvent("reset", { JsValue::Undefined(), obj });
  });

  return EventResult::kStop;
}

EventResult EventHandler::ProcessEvent(const RE::TESSellEvent* event,
                                       RE::BSTEventSource<RE::TESSellEvent>*)
{
  auto seller = event->seller.get() ? event->seller.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto sellerId = seller ? seller->formID : 0;
  auto targetId = target ? target->formID : 0;

  if ((!seller || seller->formID != sellerId) ||
      (!target || target->formID != targetId)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "seller", seller, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("sell", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSleepStartEvent* event,
  RE::BSTEventSource<RE::TESSleepStartEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "startTime", event->sleepStartTime);
    AddObjProperty(&obj, "desiredStopTime", event->desiredSleepEndTime);

    EventsApi::SendEvent("sleepStart", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSleepStopEvent* event,
  RE::BSTEventSource<RE::TESSleepStopEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "isInterrupted", event->isInterrupted);

    EventsApi::SendEvent("sleepStop", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSpellCastEvent* event,
  RE::BSTEventSource<RE::TESSpellCastEvent>*)
{
  auto caster = event->caster.get() ? event->caster.get() : nullptr;
  auto casterId = caster ? caster->formID : 0;
  auto spell = RE::TESForm::LookupByID(event->spell);

  if ((!caster || caster->formID != casterId) ||
      (spell->formType.get() != RE::FormType::Spell)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "caster", caster, "ObjectReference");
    AddObjProperty(&obj, "spell", spell, "Spell");

    EventsApi::SendEvent("spellCast", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSwitchRaceCompleteEvent* event,
  RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
{
  auto subject = event ? event->subject.get() : nullptr;
  auto subjectId = subject ? subject->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    auto subjectLocal = RE::TESForm::LookupByID(subjectId);
    subjectLocal = subjectLocal == subject ? subjectLocal : nullptr;
    AddObjProperty(&obj, "subject", subjectLocal, "ObjectReference");

    EventsApi::SendEvent("switchRaceComplete", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESTrackedStatsEvent* event,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>*)
{
  auto statName = event ? event->stat.data() : "";
  auto value = event ? event->value : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "statName", statName);
    AddObjProperty(&obj, "newValue", value);

    EventsApi::SendEvent("trackedStats", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESTriggerEnterEvent* event,
  RE::BSTEventSource<RE::TESTriggerEnterEvent>*)
{
  auto caster = event->caster.get() ? event->caster.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  if ((!caster || caster->formID != casterId) ||
      (!target || target->formID != targetId)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "caster", caster, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("triggerEnter", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESTriggerEvent* event, RE::BSTEventSource<RE::TESTriggerEvent>*)
{
  auto caster = event->caster.get() ? event->caster.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  if ((!caster || caster->formID != casterId) ||
      (!target || target->formID != targetId)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "caster", caster, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("trigger", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESTriggerLeaveEvent* event,
  RE::BSTEventSource<RE::TESTriggerLeaveEvent>*)
{
  auto caster = event->caster.get() ? event->caster.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  if ((!caster || caster->formID != casterId) ||
      (!target || target->formID != targetId)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "caster", caster, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("triggerLeave", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESUniqueIDChangeEvent* event,
  RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*)
{
  auto oldUniqueID = event ? event->oldUniqueID : 0;
  auto newUniqueID = event ? event->newUniqueID : 0;

  auto oldBaseID = event ? event->oldBaseID : 0;
  auto newBaseID = event ? event->newBaseID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "oldBaseID", oldBaseID);
    AddObjProperty(&obj, "newBaseID", newBaseID);
    AddObjProperty(&obj, "oldUniqueID", oldUniqueID);
    AddObjProperty(&obj, "tarnewUniqueIDget", newUniqueID);

    EventsApi::SendEvent("uniqueIdChange", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESWaitStartEvent* event,
  RE::BSTEventSource<RE::TESWaitStartEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "startTime", event->waitStartTime);
    AddObjProperty(&obj, "desiredStopTime", event->desiredWaitEndTime);

    EventsApi::SendEvent("waitStart", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESWaitStopEvent* event, RE::BSTEventSource<RE::TESWaitStopEvent>*)
{
  auto interrupted = event ? event->interrupted : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "isInterrupted", interrupted);

    EventsApi::SendEvent("waitStop", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const SKSE::ActionEvent* event,
  RE::BSTEventSource<SKSE::ActionEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyPtr(event);

  SkyrimPlatform::GetSingleton().AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "actor", e->actor, "Actor");
    AddObjProperty(&obj, "source", e->sourceForm, "Form");
    AddObjProperty(&obj, "slot", to_underlying(e->slot.get()));

    switch (e->type.get()) {
      case SKSE::ActionEvent::Type::kWeaponSwing: {
        SendEvent("actionWeaponSwing", obj);
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

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const SKSE::CameraEvent* event,
  RE::BSTEventSource<SKSE::CameraEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto oldStateId =
    to_underlying<RE::CameraStates::CameraState>(event->oldState->id);
  auto newStateId =
    to_underlying<RE::CameraStates::CameraState>(event->newState->id);

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "oldStateId", oldStateId);
    AddObjProperty(&obj, "newStateId", newStateId);

    EventsApi::SendEvent("cameraStateChanged", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const SKSE::CrosshairRefEvent* event,
  RE::BSTEventSource<SKSE::CrosshairRefEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto refr = event->crosshairRef ? event->crosshairRef.get() : nullptr;
  auto refrId = refr ? refr->formID : 0;

  if (!refr || refr->formID != refrId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "reference", refr, "ObjectReference");

    EventsApi::SendEvent("crosshairRefChanged", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const SKSE::NiNodeUpdateEvent* event,
  RE::BSTEventSource<SKSE::NiNodeUpdateEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto referenceId = event->reference ? event->reference->formID : 0;

  if (!event->reference || event->reference->formID != referenceId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "reference", event->reference, "ObjectReference");

    EventsApi::SendEvent("niNodeUpdate", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const SKSE::ModCallbackEvent* event,
  RE::BSTEventSource<SKSE::ModCallbackEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "sender", event->sender, "Form");
    AddObjProperty(&obj, "eventName", event->eventName.c_str());
    AddObjProperty(&obj, "strArg", event->strArg.c_str());
    AddObjProperty(&obj, "numArg", event->numArg);

    EventsApi::SendEvent("modEvent", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::MenuOpenCloseEvent* event,
  RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  const char* menuName = event->menuName.c_str();

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "name", menuName);

    if (event->opening) {
      EventsApi::SendEvent("menuOpen", { JsValue::Undefined(), obj });
    } else {
      EventsApi::SendEvent("menuClose", { JsValue::Undefined(), obj });
    }
  });

  return EventResult::kContinue;
};

EventResult EventHandler::ProcessEvent(
  const RE::BGSFootstepEvent* event, RE::BSTEventSource<RE::BGSFootstepEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "tag", event->tag.c_str());

    EventsApi::SendEvent("footstep", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::PositionPlayerEvent* event,
  RE::BSTEventSource<RE::PositionPlayerEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto type =
    to_underlying<RE::PositionPlayerEvent::EVENT_TYPE>(event->type.get());

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "eventType", type);

    EventsApi::SendEvent("positionPlayer", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::ActorKill::Event* event, RE::BSTEventSource<RE::ActorKill::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::BooksRead::Event* event, RE::BSTEventSource<RE::BooksRead::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::CriticalHit::Event* event,
  RE::BSTEventSource<RE::CriticalHit::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::DisarmedEvent::Event* event,
  RE::BSTEventSource<RE::DisarmedEvent::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::DragonSoulsGained::Event* event,
  RE::BSTEventSource<RE::DragonSoulsGained::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::ItemHarvested::Event* event,
  RE::BSTEventSource<RE::ItemHarvested::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::LevelIncrease::Event* event,
  RE::BSTEventSource<RE::LevelIncrease::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::LocationDiscovery::Event* event,
  RE::BSTEventSource<RE::LocationDiscovery::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::ShoutAttack::Event* event,
  RE::BSTEventSource<RE::ShoutAttack::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::SkillIncrease::Event* event,
  RE::BSTEventSource<RE::SkillIncrease::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::SoulsTrapped::Event* event,
  RE::BSTEventSource<RE::SoulsTrapped::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::SpellsLearned::Event* event,
  RE::BSTEventSource<RE::SpellsLearned::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}
