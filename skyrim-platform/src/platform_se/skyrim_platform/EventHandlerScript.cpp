#include "EventHandlerScript.h"
#include "EventsApi.h"
#include "NativeValueCasts.h"
#include "SkyrimPlatform.h"

namespace {
JsValue CreateObject(const char* type, void* form)
{
  return form ? NativeValueCasts::NativeObjectToJsObject(
                  std::make_shared<CallNative::Object>(type, form))
              : JsValue::Null();
}
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESActivateEvent* event, RE::BSTEventSource<RE::TESActivateEvent>*)
{
  auto targetRefr = event ? event->objectActivated.get() : nullptr;
  auto casterRefr = event ? event->actionRef.get() : nullptr;

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

    obj.SetProperty(
      "effect", CreateObject("MagicEffect", activeEffect->GetBaseObject()));

    obj.SetProperty("activeEffect",
                    CreateObject("ActiveMagicEffect", activeEffect));

    obj.SetProperty("caster", CreateObject("ObjectReference", caster));
    obj.SetProperty("target", CreateObject("ObjectReference", target));

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

    obj.SetProperty("actor", CreateObject("Actor", actor));
    obj.SetProperty("oldLoc", CreateObject("Location", event->oldLoc));
    obj.SetProperty("newLoc", CreateObject("Location", event->newLoc));

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

    obj.SetProperty("book", CreateObject("ObjectReference", book));

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

  SkyrimPlatform::GetSingleton().AddUpdateTask([refr, action] {
    auto obj = JsValue::Object();

    obj.SetProperty("refr", CreateObject("ObjectReference", refr));

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

  SkyrimPlatform::GetSingleton().AddUpdateTask([cellId, cell] {
    auto obj = JsValue::Object();

    auto cell_ = RE::TESForm::LookupByID(cellId);
    cell_ = cell_ == cell ? cell_ : nullptr;
    obj.SetProperty("cell", CreateObject("Cell", cell_));

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
        JsValue::Bool(state.get() == RE::ACTOR_COMBAT_STATE::kCombat));

      obj.SetProperty(
        "isSearching",
        JsValue::Bool(state.get() == RE::ACTOR_COMBAT_STATE::kSearching));

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

    obj.SetProperty("target", CreateObject("ObjectReference", target));
    obj.SetProperty("oldStage", JsValue::Double(event->oldStage));
    obj.SetProperty("newStage", JsValue::Double(event->newStage));

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

    obj.SetProperty("actor", CreateObject("ObjectReference", actor));

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

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESFastTravelEndEvent* event,
  RE::BSTEventSource<RE::TESFastTravelEndEvent>*)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    obj.SetProperty("travelTimeGameHours",
                    JsValue::Double(event->travelTimeGameHours));

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

    obj.SetProperty("actor", CreateObject("ObjectReference", actor));
    obj.SetProperty("target", CreateObject("ObjectReference", target));

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

  SkyrimPlatform::GetSingleton().AddUpdateTask([refId, grabbed, ref] {
    auto obj = JsValue::Object();

    auto reference = RE::TESForm::LookupByID(refId);
    reference = reference == ref ? reference : nullptr;
    obj.SetProperty("refr", CreateObject("ObjectReference", reference));

    obj.SetProperty("isGrabbed", JsValue::Bool(grabbed));

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
        JsValue::Bool(flags.any(RE::TESHitEvent::Flag::kPowerAttack)));

      obj.SetProperty(
        "isSneakAttack",
        JsValue::Bool(flags.any(RE::TESHitEvent::Flag::kSneakAttack)));

      obj.SetProperty(
        "isBashAttack",
        JsValue::Bool(flags.any(RE::TESHitEvent::Flag::kBashAttack)));

      obj.SetProperty(
        "isHitBlocked",
        JsValue::Bool(flags.any(RE::TESHitEvent::Flag::kHitBlocked)));

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
    obj.SetProperty("lockedObject",
                    CreateObject("ObjectReference", lockedObject));

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

    obj.SetProperty("effect", CreateObject("MagicEffect", effect));
    obj.SetProperty("caster", CreateObject("ObjectReference", caster));
    obj.SetProperty("target", CreateObject("ObjectReference", target));

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

    obj.SetProperty("caster", CreateObject("ObjectReference", caster));
    obj.SetProperty("target", CreateObject("ObjectReference", target));
    obj.SetProperty("spell", CreateObject("Spell", spell));

    switch (event->status) {
      case RE::TESMagicWardHitEvent::Status::kFriendly: {
        obj.SetProperty("status", "friendly");
        break;
      }
      case RE::TESMagicWardHitEvent::Status::kAbsorbed: {
        obj.SetProperty("status", "absorbed");
        break;
      }
      case RE::TESMagicWardHitEvent::Status::kBroken: {
        obj.SetProperty("status", "broken");
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

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [targetId, isCellAttached, movedRef] {
      auto obj = JsValue::Object();

      auto target = RE::TESForm::LookupByID(targetId);
      target = target == movedRef ? target : nullptr;
      obj.SetProperty("movedRef", CreateObject("ObjectReference", target));

      obj.SetProperty("isCellAttached", JsValue::Bool(isCellAttached));
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

  SkyrimPlatform::GetSingleton().AddUpdateTask([objectId, loaded] {
    auto obj = JsValue::Object();

    obj.SetProperty("object",
                    CreateObject("Form", RE::TESForm::LookupByID(objectId)));

    obj.SetProperty("isLoaded", JsValue::Bool(loaded));

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

    obj.SetProperty("reference", CreateObject("ObjectReference", refLocal));

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

    obj.SetProperty("caster", CreateObject("ObjectReference", caster));
    obj.SetProperty("target", CreateObject("ObjectReference", target));

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

    obj.SetProperty("actor", CreateObject("ObjectReference", actor));
    obj.SetProperty("package", CreateObject("Package", package));

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

    obj.SetProperty("cause", CreateObject("ObjectReference", cause));
    obj.SetProperty("target", CreateObject("ObjectReference", target));
    obj.SetProperty("perk", CreateObject("Perk", perk));
    obj.SetProperty("flag", JsValue::Double(event->flag));

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

    obj.SetProperty("weapon", CreateObject("Weapon", weapon));
    obj.SetProperty("ammo", CreateObject("Ammo", ammo));
    obj.SetProperty("power", JsValue::Double(event->power));
    obj.SetProperty("isSunGazing", JsValue::Bool(event->isSunGazing));

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

  SkyrimPlatform::GetSingleton().AddUpdateTask([quest] {
    auto obj = JsValue::Object();

    obj.SetProperty("quest", CreateObject("Quest", quest));

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

    obj.SetProperty("quest", CreateObject("Quest", quest));
    obj.SetProperty("stage", JsValue::Double(event->stage));

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

    obj.SetProperty("quest", CreateObject("Quest", quest));

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

  SkyrimPlatform::GetSingleton().AddUpdateTask([objectId, object] {
    auto obj = JsValue::Object();

    auto objectIdLocal = RE::TESForm::LookupByID(objectId);
    objectIdLocal = objectIdLocal == object ? objectIdLocal : nullptr;

    obj.SetProperty("object", CreateObject("ObjectReference", objectIdLocal));

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

    obj.SetProperty("seller", CreateObject("ObjectReference", seller));
    obj.SetProperty("target", CreateObject("ObjectReference", target));

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

    obj.SetProperty("startTime", JsValue::Double(event->sleepStartTime));
    obj.SetProperty("desiredStopTime",
                    JsValue::Double(event->desiredSleepEndTime));

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

    obj.SetProperty("isInterrupted", JsValue::Bool(event->isInterrupted));

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

    obj.SetProperty("caster", CreateObject("ObjectReference", caster));
    obj.SetProperty("spell", CreateObject("Spell", spell));

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

  SkyrimPlatform::GetSingleton().AddUpdateTask([subjectId, subject] {
    auto obj = JsValue::Object();

    auto subjectLocal = RE::TESForm::LookupByID(subjectId);
    subjectLocal = subjectLocal == subject ? subjectLocal : nullptr;
    obj.SetProperty("subject", CreateObject("ObjectReference", subjectLocal));

    EventsApi::SendEvent("switchRaceComplete", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESTrackedStatsEvent* event,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>*)
{
  std::string statName = event ? event->stat.data() : "";
  auto value = event ? event->value : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([statName, value] {
    auto obj = JsValue::Object();

    obj.SetProperty("statName", JsValue::String(statName));
    obj.SetProperty("newValue", JsValue::Double(value));

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

    obj.SetProperty("caster", CreateObject("ObjectReference", caster));
    obj.SetProperty("target", CreateObject("ObjectReference", target));

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

    obj.SetProperty("caster", CreateObject("ObjectReference", caster));
    obj.SetProperty("target", CreateObject("ObjectReference", target));

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

    obj.SetProperty("caster", CreateObject("ObjectReference", caster));
    obj.SetProperty("target", CreateObject("ObjectReference", target));

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

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [oldUniqueID, newUniqueID, oldBaseID, newBaseID] {
      auto obj = JsValue::Object();

      obj.SetProperty("oldBaseID", JsValue::Double(oldBaseID));
      obj.SetProperty("newBaseID", JsValue::Double(newBaseID));
      obj.SetProperty("oldUniqueID", JsValue::Double(oldUniqueID));
      obj.SetProperty("newUniqueID", JsValue::Double(newUniqueID));

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

    obj.SetProperty("startTime", JsValue::Double(event->waitStartTime));
    obj.SetProperty("desiredStopTime",
                    JsValue::Double(event->desiredWaitEndTime));

    EventsApi::SendEvent("waitStart", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerScript::ProcessEvent(
  const RE::TESWaitStopEvent* event, RE::BSTEventSource<RE::TESWaitStopEvent>*)
{
  auto interrupted = event ? event->interrupted : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([interrupted] {
    auto obj = JsValue::Object();

    obj.SetProperty("isInterrupted", JsValue::Bool(interrupted));

    EventsApi::SendEvent("waitStop", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}
