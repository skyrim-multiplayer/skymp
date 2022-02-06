#include "EventHandlerScript.h"
#include "EventUtils.h"
#include "../EventsApi.h"
#include "../SkyrimPlatform.h"

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESActivateEvent* event, RE::BSTEventSource<RE::TESActivateEvent>*)
{
  auto targetRefr = event ? event->objectActivated.get() : nullptr;
  auto casterRefr = event ? event->actionRef.get() : nullptr;

  auto targetId = targetRefr ? targetRefr->formID : 0;
  auto casterId = casterRefr ? casterRefr->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto target = RE::TESForm::LookupByID(targetId);
    target = target == targetRefr ? target : nullptr;
    AddProperty(&obj, "target", target, "ObjectReference");

    auto caster = RE::TESForm::LookupByID(casterId);
    caster = caster == casterRefr ? caster : nullptr;
    AddProperty(&obj, "caster", caster, "ObjectReference");

    AddProperty(&obj, "isCrimeToActivate",
                target->As<RE::TESObjectREFR>()->IsCrimeToActivate());

    EventsApi::SendEvent("activate", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "effect", activeEffect->GetBaseObject(), "MagicEffect");
    AddProperty(&obj, "activeEffect", activeEffect, "ActiveMagicEffect");
    AddProperty(&obj, "caster", caster, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");

    if (event->isApplied)
      EventsApi::SendEvent("effectStart", { JsValue::Undefined(), obj });
    else
      EventsApi::SendEvent("effectFinish", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESActorLocationChangeEvent* event,
  RE::BSTEventSource<RE::TESActorLocationChangeEvent>*)
{
  auto actor = event->actor.get() ? event->actor.get() : nullptr;

  auto actorId = actor ? actor->formID : 0;

  if ((!actor || actor->formID != actorId) ||
      (event->oldLoc->formType != RE::FormType::Location) ||
      (event->newLoc->formType != RE::FormType::Location)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "actor", actor, "Actor");
    AddProperty(&obj, "oldLoc", event->oldLoc, "Location");
    AddProperty(&obj, "newLoc", event->newLoc, "Location");

    EventsApi::SendEvent("locationChanged", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESBookReadEvent* event, RE::BSTEventSource<RE::TESBookReadEvent>*)
{
  auto book = event->book.get() ? event->book.get() : nullptr;
  auto bookId = book ? book->formID : 0;

  if ((!book || book->formID != bookId) ||
      (book->formType != RE::FormType::Book)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "book", book, "ObjectReference");

    EventsApi::SendEvent("bookRead", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESCellAttachDetachEvent* event,
  RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
{
  auto refr = event->reference.get();
  auto action = event->action;

  auto formId = refr ? refr->formID : 0;
  auto form = RE::TESForm::LookupByID(formId);

  if (!form || refr->formID != form->formID) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "refr", refr, "ObjectReference");

    if (action == 1)
      EventsApi::SendEvent("cellAttach", { JsValue::Undefined(), obj });

    if (action == 0)
      EventsApi::SendEvent("cellDetach", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESCellFullyLoadedEvent* event,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*)
{
  auto cell = event ? event->cell : nullptr;
  auto cellId = cell ? cell->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto cell_ = RE::TESForm::LookupByID(cellId);
    cell_ = cell_ == cell ? cell_ : nullptr;
    AddProperty(&obj, "cell", cell_, "Cell");

    EventsApi::SendEvent("cellFullyLoaded", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>*)
{
  auto targetActorRefr = event ? event->targetActor.get() : nullptr;
  auto targetActorId = targetActorRefr ? targetActorRefr->formID : 0;

  auto actorRefr = event ? event->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto state = event
    ? event->newState
    : RE::stl::enumeration<RE::ACTOR_COMBAT_STATE, std::uint32_t>(
        RE::ACTOR_COMBAT_STATE::kNone);

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto targetActorLocal = RE::TESForm::LookupByID(targetActorId);
    targetActorLocal =
      targetActorLocal == targetActorRefr ? targetActorLocal : nullptr;
    AddProperty(&obj, "target", targetActorLocal, "ObjectReference");

    auto actorLocal = RE::TESForm::LookupByID(actorId);
    actorLocal = actorLocal == actorRefr ? actorLocal : nullptr;
    AddProperty(&obj, "actor", actorLocal, "ObjectReference");

    AddProperty(&obj, "isCombat",
                state.get() == RE::ACTOR_COMBAT_STATE::kCombat);
    AddProperty(&obj, "isSearching",
                state.get() == RE::ACTOR_COMBAT_STATE::kSearching);

    EventsApi::SendEvent("combatState", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "oldContainer", RE::TESForm::LookupByID(oldContainerId),
                "ObjectReference");
    AddProperty(&obj, "newContainer", RE::TESForm::LookupByID(newContainerId),
                "ObjectReference");
    AddProperty(&obj, "baseObj", RE::TESForm::LookupByID(baseObjId), "Form");
    AddProperty(&obj, "numItems", itemCount);
    AddProperty(&obj, "uniqueID", uniqueID);
    AddProperty(&obj, "reference", RE::TESForm::LookupByID(referenceId),
                "ObjectReference");

    EventsApi::SendEvent("containerChanged", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESDeathEvent* event, RE::BSTEventSource<RE::TESDeathEvent>*)
{
  auto actorDyingRefr = event ? event->actorDying.get() : nullptr;
  auto actorDyingId = actorDyingRefr ? actorDyingRefr->formID : 0;

  auto actorKillerRefr = event ? event->actorKiller.get() : nullptr;
  auto actorKillerId = actorKillerRefr ? actorKillerRefr->formID : 0;

  auto dead = event ? event->dead : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto actorDyingLocal = RE::TESForm::LookupByID(actorDyingId);
    actorDyingLocal =
      actorDyingLocal == actorDyingRefr ? actorDyingLocal : nullptr;

    AddProperty(&obj, "actorDying", actorDyingLocal, "ObjectReference");

    auto actorKillerLocal = RE::TESForm::LookupByID(actorKillerId);
    actorKillerLocal =
      actorKillerLocal == actorKillerRefr ? actorKillerLocal : nullptr;

    AddProperty(&obj, "actorKiller", actorKillerLocal, "ObjectReference");

    dead ? EventsApi::SendEvent("deathEnd", { JsValue::Undefined(), obj })
         : EventsApi::SendEvent("deathStart", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESDestructionStageChangedEvent* event,
  RE::BSTEventSource<RE::TESDestructionStageChangedEvent>*)
{
  auto target = event->target ? event->target.get() : nullptr;
  auto targetId = target ? target->formID : 0;

  if (!target || target->formID != targetId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "target", target, "ObjectReference");
    AddProperty(&obj, "oldStage", event->oldStage);
    AddProperty(&obj, "newStage", event->newStage);

    EventsApi::SendEvent("destructionStageChanged",
                         { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESEnterBleedoutEvent* event,
  RE::BSTEventSource<RE::TESEnterBleedoutEvent>*)
{
  auto actor = event->actor.get() ? event->actor.get() : nullptr;
  auto actorId = actor ? actor->formID : 0;

  if (!actor || actor->formID != actorId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "actor", actor, "ObjectReference");

    EventsApi::SendEvent("enterBleedout", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*)
{
  auto actorRefr = event ? event->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto originalRefrId = event ? event->originalRefr : 0;
  auto baseObjectId = event ? event->baseObject : 0;
  auto equipped = event ? event->equipped : 0;
  auto uniqueId = event ? event->uniqueID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto actorLocal = RE::TESForm::LookupByID(actorId);
    actorLocal = actorLocal == actorRefr ? actorLocal : nullptr;
    AddProperty(&obj, "actor", actorLocal, "ObjectReference");

    AddProperty(&obj, "baseObj", RE::TESForm::LookupByID(baseObjectId),
                "Form");
    AddProperty(&obj, "originalRefr", RE::TESForm::LookupByID(originalRefrId),
                "ObjectReference");
    AddProperty(&obj, "uniqueId", uniqueId);

    equipped ? EventsApi::SendEvent("equip", { JsValue::Undefined(), obj })
             : EventsApi::SendEvent("unequip", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESFastTravelEndEvent* event,
  RE::BSTEventSource<RE::TESFastTravelEndEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "travelTimeGameHours", event->travelTimeGameHours);

    EventsApi::SendEvent("fastTravelEnd", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "actor", actor, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");

    if (event->type == RE::TESFurnitureEvent::FurnitureEventType::kExit) {
      EventsApi::SendEvent("furnitureExit", { JsValue::Undefined(), obj });
    } else if (event->type ==
               RE::TESFurnitureEvent::FurnitureEventType::kEnter) {
      EventsApi::SendEvent("furnitureEnter", { JsValue::Undefined(), obj });
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESGrabReleaseEvent* event,
  RE::BSTEventSource<RE::TESGrabReleaseEvent>*)
{
  auto ref = event ? event->ref.get() : nullptr;
  auto refId = ref ? ref->formID : 0;
  auto grabbed = event ? event->grabbed : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto reference = RE::TESForm::LookupByID(refId);
    reference = reference == ref ? reference : nullptr;
    AddProperty(&obj, "refr", reference, "ObjectReference");

    AddProperty(&obj, "isGrabbed", grabbed);

    EventsApi::SendEvent("grabRelease", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESHitEvent* event, RE::BSTEventSource<RE::TESHitEvent>*)
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto targetLocal = RE::TESForm::LookupByID(targetId);
    targetLocal = targetLocal == targetRefr ? targetLocal : nullptr;
    AddProperty(&obj, "target", targetLocal, "ObjectReference");

    auto causeLocal = RE::TESForm::LookupByID(causeId);
    causeLocal = causeLocal == causeRefr ? causeLocal : nullptr;
    // TODO(#336): drop old name "agressor" on next major release of SP
    AddProperty(&obj, "agressor", causeLocal, "ObjectReference");

    AddProperty(&obj, "aggressor", causeLocal, "ObjectReference");
    AddProperty(&obj, "source", RE::TESForm::LookupByID(sourceId), "Form");
    AddProperty(&obj, "projectile", RE::TESForm::LookupByID(projectileId),
                "Form");
    AddProperty(&obj, "isPowerAttack",
                flags.any(RE::TESHitEvent::Flag::kPowerAttack));
    AddProperty(&obj, "isSneakAttack",
                flags.any(RE::TESHitEvent::Flag::kSneakAttack));
    AddProperty(&obj, "isBashAttack",
                flags.any(RE::TESHitEvent::Flag::kBashAttack));
    AddProperty(&obj, "isHitBlocked",
                flags.any(RE::TESHitEvent::Flag::kHitBlocked));

    EventsApi::SendEvent("hit", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESInitScriptEvent* event,
  RE::BSTEventSource<RE::TESInitScriptEvent>*)
{
  auto objectInitialized = event ? event->objectInitialized.get() : nullptr;
  auto objectInitializedId = objectInitialized ? objectInitialized->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto objectInitializedLocal = RE::TESForm::LookupByID(objectInitializedId);
    objectInitializedLocal = objectInitializedLocal == objectInitialized
      ? objectInitializedLocal
      : nullptr;
    AddProperty(&obj, "initializedObject", objectInitializedLocal,
                "ObjectReference");

    EventsApi::SendEvent("scriptInit", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

// TODO: Look into LoadGame event
EventResult EventHandlerScript::ProcessEvent(
  const RE::TESLoadGameEvent* event, RE::BSTEventSource<RE::TESLoadGameEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [] { EventsApi::SendEvent("loadGame", { JsValue::Undefined() }); });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESLockChangedEvent* event,
  RE::BSTEventSource<RE::TESLockChangedEvent>*)
{
  auto lockedObject = event ? event->lockedObject : nullptr;
  auto lockedObjectId = lockedObject ? lockedObject->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([lockedObjectId, lockedObject] {
    auto obj = JsValue::Object();

    auto lockedObject = RE::TESForm::LookupByID(lockedObjectId);
    lockedObject = lockedObject == lockedObject ? lockedObject : nullptr;
    AddProperty(&obj, "lockedObject", lockedObject, "ObjectReference");

    EventsApi::SendEvent("lockChanged", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "effect", effect, "MagicEffect");
    AddProperty(&obj, "caster", caster, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("magicEffectApply", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "caster", caster, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");
    AddProperty(&obj, "spell", spell, "Spell");

    switch (event->status) {
      case RE::TESMagicWardHitEvent::Status::kFriendly: {
        AddProperty(&obj, "status", "friendly");
        break;
      }
      case RE::TESMagicWardHitEvent::Status::kAbsorbed: {
        AddProperty(&obj, "status", "absorbed");
        break;
      }
      case RE::TESMagicWardHitEvent::Status::kBroken: {
        AddProperty(&obj, "status", "broken");
        break;
      }
    }

    EventsApi::SendEvent("wardHit", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESMoveAttachDetachEvent* event,
  RE::BSTEventSource<RE::TESMoveAttachDetachEvent>*)
{
  auto movedRef = event ? event->movedRef.get() : nullptr;

  auto targetId = movedRef ? movedRef->formID : 0;
  auto isCellAttached = event ? event->isCellAttached : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto target = RE::TESForm::LookupByID(targetId);
    target = target == movedRef ? target : nullptr;
    AddProperty(&obj, "movedRef", target, "ObjectReference");

    AddProperty(&obj, "isCellAttached", isCellAttached);

    EventsApi::SendEvent("moveAttachDetach", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESObjectLoadedEvent* event,
  RE::BSTEventSource<RE::TESObjectLoadedEvent>*)
{
  auto objectId = event ? event->formID : 0;
  auto loaded = event ? event->loaded : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "object", RE::TESForm::LookupByID(objectId), "Form");
    AddProperty(&obj, "isLoaded", loaded);

    EventsApi::SendEvent("objectLoaded", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

    AddProperty(&obj, "reference", refLocal, "ObjectReference");

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

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "caster", caster, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");

    if (event->opened) {
      EventsApi::SendEvent("open", { JsValue::Undefined(), obj });
    } else {
      EventsApi::SendEvent("close", { JsValue::Undefined(), obj });
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESPackageEvent* event, RE::BSTEventSource<RE::TESPackageEvent>*)
{
  auto actor = event->actor.get() ? event->actor.get() : nullptr;
  auto actorId = actor ? actor->formID : 0;

  auto package = RE::TESForm::LookupByID(event->package);
  if ((!actor || actor->formID != actorId) ||
      (package->formType.get() != RE::FormType::Package)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "actor", actor, "ObjectReference");
    AddProperty(&obj, "package", package, "Package");

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

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "cause", cause, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");
    AddProperty(&obj, "perk", perk, "Perk");
    AddProperty(&obj, "flag", event->flag);

    EventsApi::SendEvent("perkEntryRun", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESPlayerBowShotEvent* event,
  RE::BSTEventSource<RE::TESPlayerBowShotEvent>*)
{
  auto weapon = RE::TESForm::LookupByID(event->weaponId);
  auto ammo = RE::TESForm::LookupByID(event->ammoId);

  if (weapon->formType.get() != RE::FormType::Weapon ||
      ammo->formType.get() != RE::FormType::Ammo) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "weapon", weapon, "Weapon");
    AddProperty(&obj, "ammo", ammo, "Ammo");
    AddProperty(&obj, "power", event->power);
    AddProperty(&obj, "target", event->isSunGazing);

    EventsApi::SendEvent("playerBowShot", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESQuestInitEvent* event,
  RE::BSTEventSource<RE::TESQuestInitEvent>*)
{
  auto quest = RE::TESForm::LookupByID(event->questId);

  if (quest->formType.get() != RE::FormType::Quest)
    return EventResult::kContinue;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "quest", quest, "Quest");

    EventsApi::SendEvent("questInit", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESQuestStageEvent* event,
  RE::BSTEventSource<RE::TESQuestStageEvent>*)
{
  auto quest = RE::TESForm::LookupByID(event->questId);

  if (quest->formType.get() != RE::FormType::Quest)
    return EventResult::kContinue;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "quest", quest, "Quest");
    AddProperty(&obj, "stage", event->stage);

    EventsApi::SendEvent("questStage", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESQuestStartStopEvent* event,
  RE::BSTEventSource<RE::TESQuestStartStopEvent>*)
{
  auto quest = RE::TESForm::LookupByID(event->questId);

  if (quest->formType.get() != RE::FormType::Quest)
    return EventResult::kContinue;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "quest", quest, "Quest");

    if (event->isStarted) {
      EventsApi::SendEvent("questStart", { JsValue::Undefined(), obj });
    } else {
      EventsApi::SendEvent("questStop", { JsValue::Undefined(), obj });
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESResetEvent* event, RE::BSTEventSource<RE::TESResetEvent>*)
{
  auto object = event ? event->object.get() : nullptr;
  auto objectId = object ? object->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto objectIdLocal = RE::TESForm::LookupByID(objectId);
    objectIdLocal = objectIdLocal == object ? objectIdLocal : nullptr;
    AddProperty(&obj, "object", objectIdLocal, "ObjectReference");

    EventsApi::SendEvent("reset", { JsValue::Undefined(), obj });
  });

  return EventResult::kStop;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESSellEvent* event, RE::BSTEventSource<RE::TESSellEvent>*)
{
  auto seller = event->seller.get() ? event->seller.get() : nullptr;
  auto target = event->target.get() ? event->target.get() : nullptr;

  auto sellerId = seller ? seller->formID : 0;
  auto targetId = target ? target->formID : 0;

  if ((!seller || seller->formID != sellerId) ||
      (!target || target->formID != targetId)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "seller", seller, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("sell", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESSleepStartEvent* event,
  RE::BSTEventSource<RE::TESSleepStartEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "startTime", event->sleepStartTime);
    AddProperty(&obj, "desiredStopTime", event->desiredSleepEndTime);

    EventsApi::SendEvent("sleepStart", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESSleepStopEvent* event,
  RE::BSTEventSource<RE::TESSleepStopEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "isInterrupted", event->isInterrupted);

    EventsApi::SendEvent("sleepStop", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "caster", caster, "ObjectReference");
    AddProperty(&obj, "spell", spell, "Spell");

    EventsApi::SendEvent("spellCast", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESSwitchRaceCompleteEvent* event,
  RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
{
  auto subject = event ? event->subject.get() : nullptr;
  auto subjectId = subject ? subject->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    auto subjectLocal = RE::TESForm::LookupByID(subjectId);
    subjectLocal = subjectLocal == subject ? subjectLocal : nullptr;
    AddProperty(&obj, "subject", subjectLocal, "ObjectReference");

    EventsApi::SendEvent("switchRaceComplete", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESTrackedStatsEvent* event,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>*)
{
  auto statName = event ? event->stat.data() : "";
  auto value = event ? event->value : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "statName", statName);
    AddProperty(&obj, "newValue", value);

    EventsApi::SendEvent("trackedStats", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "caster", caster, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("triggerEnter", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "caster", caster, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("trigger", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "caster", caster, "ObjectReference");
    AddProperty(&obj, "target", target, "ObjectReference");

    EventsApi::SendEvent("triggerLeave", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESUniqueIDChangeEvent* event,
  RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*)
{
  auto oldUniqueID = event ? event->oldUniqueID : 0;
  auto newUniqueID = event ? event->newUniqueID : 0;

  auto oldBaseID = event ? event->oldBaseID : 0;
  auto newBaseID = event ? event->newBaseID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "oldBaseID", oldBaseID);
    AddProperty(&obj, "newBaseID", newBaseID);
    AddProperty(&obj, "oldUniqueID", oldUniqueID);
    AddProperty(&obj, "tarnewUniqueIDget", newUniqueID);

    EventsApi::SendEvent("uniqueIdChange", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESWaitStartEvent* event,
  RE::BSTEventSource<RE::TESWaitStartEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "startTime", event->waitStartTime);
    AddProperty(&obj, "desiredStopTime", event->desiredWaitEndTime);

    EventsApi::SendEvent("waitStart", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESWaitStopEvent* event, RE::BSTEventSource<RE::TESWaitStopEvent>*)
{
  auto interrupted = event ? event->interrupted : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "isInterrupted", interrupted);

    EventsApi::SendEvent("waitStop", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}
