#include "EventHandler.h"
#include "EventManager.h"
#include "EventsApi.h"
#include "JsUtils.h"
#include "SkyrimPlatform.h"

namespace {
inline void SendEvent(const char* eventName)
{
  EventsApi::SendEvent(eventName, { JsValue::Undefined() });
}

inline void SendEvent(const char* eventName, const JsValue& obj)
{
  EventsApi::SendEvent(eventName, { JsValue::Undefined(), obj });
}
}

void EventHandler::SendSimpleEventOnUpdate(const char* eventName)
{
  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [eventName] { SendEvent(eventName); });
}

void EventHandler::SendSimpleEventOnTick(const char* eventName)
{
  SkyrimPlatform::GetSingleton()->AddTickTask(
    [eventName] { SendEvent(eventName); });
}

void EventHandler::SendEventOnUpdate(const char* eventName, const JsValue& obj)
{
  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [eventName, obj] { SendEvent(eventName, obj); });
}

void EventHandler::SendEventOnTick(const char* eventName, const JsValue& obj)
{
  SkyrimPlatform::GetSingleton()->AddTickTask(
    [eventName, obj] { SendEvent(eventName, obj); });
}

void EventHandler::SendEventConsoleMsg(const char* msg)
{
  SkyrimPlatform::GetSingleton()->AddTickTask([msg] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "message", msg);

    SendEvent("consoleMessage", obj);
  });
}

void EventHandler::HandleSKSEMessage(SKSE::MessagingInterface::Message* msg)
{
  switch (msg->type) {
    case SKSE::MessagingInterface::kDataLoaded: {
      EventManager::Init();
      SendSimpleEventOnTick("skyrimLoaded");
    } break;
    case SKSE::MessagingInterface::kNewGame:
      SendSimpleEventOnTick("newGame");
      break;
    case SKSE::MessagingInterface::kPreLoadGame:
      SendSimpleEventOnTick("preLoadGame");
      break;
    case SKSE::MessagingInterface::kPostLoadGame:
      SendSimpleEventOnTick("postLoadGame");
      break;
    case SKSE::MessagingInterface::kSaveGame:
      SendSimpleEventOnTick("saveGame");
      break;
    case SKSE::MessagingInterface::kDeleteGame:
      SendSimpleEventOnTick("deleteGame");
      break;
  }
}

EventResult EventHandler::ProcessEvent(
  const RE::TESActivateEvent* event, RE::BSTEventSource<RE::TESActivateEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto targetId =
    event->objectActivated ? event->objectActivated->GetFormID() : 0;
  auto casterId = event->actionRef ? event->actionRef->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([targetId, casterId] {
    auto obj = JsValue::Object();

    auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);
    auto caster = RE::TESForm::LookupByID<RE::TESObjectREFR>(casterId);

    if (target && caster) {
      AddObjProperty(&obj, "target", target, "ObjectReference");
      AddObjProperty(&obj, "isCrimeToActivate", target->IsCrimeToActivate());
      AddObjProperty(&obj, "caster", caster, "ObjectReference");

      SendEvent("activate", obj);
    }
  });

  return EventResult::kContinue;
}

// TODO: take a look at this code. seems to be incorrect.
EventResult EventHandler::ProcessEvent(
  const RE::TESActiveEffectApplyRemoveEvent* event,
  RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyEventPtr(event);

  /**
   * this is a workaround
   * at the moment of writing if you try to create new instance of
   * RE::ActiveEffect the game crashes either at instance construction or
   * deconstruction, but since we really need copies of those classes here we
   * have to manually manage memory to avoid leaks
   */
  auto effectList = std::make_shared<std::vector<std::unique_ptr<
    RE::ActiveEffect, game_type_pointer_deleter<RE::ActiveEffect>>>>();

  for (const auto& eff :
       *event->target.get()->As<RE::Actor>()->GetActiveEffectList()) {
    if (eff->usUniqueID != 0) {
      auto activeEffect = RE::malloc<RE::ActiveEffect>();
      std::memcpy(activeEffect, eff, sizeof(*eff));
      effectList->emplace_back(activeEffect,
                               game_type_pointer_deleter<RE::ActiveEffect>());
    }
  }

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e, effectList] {
    for (const auto& effect : *effectList.get()) {
      if (effect->usUniqueID == e->activeEffectUniqueID) {
        auto obj = JsValue::Object();

        AddObjProperty(&obj, "effect", effect->GetBaseObject(), "MagicEffect");
        AddObjProperty(&obj, "caster", e->caster.get(), "ObjectReference");
        AddObjProperty(&obj, "target", e->target.get(), "ObjectReference");

        if (e->isApplied) {
          SendEvent("effectStart", obj);
        } else {
          SendEvent("effectFinish", obj);
        }

        return;
      }
    }
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

  auto actorId = event->actor ? event->actor->GetFormID() : 0;
  auto oldLocId = event->oldLoc ? event->oldLoc->GetFormID() : 0;
  auto newLocId = event->newLoc ? event->newLoc->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([actorId, oldLocId, newLocId] {
    auto obj = JsValue::Object();

    auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);
    auto oldLoc = RE::TESForm::LookupByID<RE::BGSLocation>(oldLocId);
    auto newLoc = RE::TESForm::LookupByID<RE::BGSLocation>(newLocId);

    if (actor && oldLoc && newLoc) {
      AddObjProperty(&obj, "actor", actor, "Actor");
      AddObjProperty(&obj, "oldLoc", oldLoc, "Location");
      AddObjProperty(&obj, "newLoc", newLoc, "Location");

      SendEvent("locationChanged", obj);
    }
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

  RE::TESObjectREFR* refr = event->reference.get();
  if (!refr) {
    return EventResult::kContinue;
  }
  auto refrId = refr->GetFormID();

  bool isAttach = event->action == 1;
  if (isAttach) {
    SkyrimPlatform::GetSingleton()->AddUpdateTask([refrId] {
      auto obj = JsValue::Object();
      auto refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(refrId);
      if (refr) {
        AddObjProperty(&obj, "refr", refr, "ObjectReference");
        SendEvent("cellAttach", obj);
      }
    });
    return EventResult::kContinue;
  }

  bool isDetach = event->action == 0;
  if (isDetach) {
    SkyrimPlatform::GetSingleton()->AddUpdateTask([refrId] {
      auto obj = JsValue::Object();
      auto refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(refrId);
      if (refr) {
        AddObjProperty(&obj, "refr", refr, "ObjectReference");
        SendEvent("cellDetach", obj);
      }
    });
    return EventResult::kContinue;
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESCellFullyLoadedEvent* event,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto cellId = event->cell ? event->cell->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([cellId] {
    auto obj = JsValue::Object();

    auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(cellId);
    if (cell) {
      AddObjProperty(&obj, "cell", cell, "Cell");
      SendEvent("cellFullyLoaded", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESCombatEvent* event,
                                       RE::BSTEventSource<RE::TESCombatEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto targetActorId =
    event->targetActor ? event->targetActor->GetFormID() : 0;
  auto actorId = event->actor ? event->actor->GetFormID() : 0;
  bool isCombat = event->newState.any(RE::ACTOR_COMBAT_STATE::kCombat);
  bool isSearching = event->newState.any(RE::ACTOR_COMBAT_STATE::kSearching);

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [targetActorId, actorId, isCombat, isSearching] {
      auto obj = JsValue::Object();

      auto targetActor =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(targetActorId);
      auto actor = RE::TESForm::LookupByID<RE::TESObjectREFR>(actorId);

      if (targetActor && actor) {
        AddObjProperty(&obj, "target", targetActor, "ObjectReference");
        AddObjProperty(&obj, "actor", actor, "ObjectReference");
        AddObjProperty(&obj, "isCombat", isCombat);
        AddObjProperty(&obj, "isSearching", isSearching);

        SendEvent("combatState", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESContainerChangedEvent* event,
  RE::BSTEventSource<RE::TESContainerChangedEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto oldContainerId = event->oldContainer;
  auto newContainerId = event->newContainer;
  auto baseObjId = event->baseObj;
  auto referenceId =
    event->reference ? event->reference.get()->GetFormID() : 0;
  auto itemCount = event->itemCount;
  auto uniqueID = event->uniqueID;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [oldContainerId, newContainerId, baseObjId, referenceId, itemCount,
     uniqueID] {
      auto obj = JsValue::Object();

      auto oldContainer =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(oldContainerId);
      auto newContainer =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(newContainerId);
      auto baseObj = RE::TESForm::LookupByID(baseObjId);
      auto reference = RE::TESForm::LookupByID<RE::TESObjectREFR>(referenceId);

      if ((oldContainerId == 0 || oldContainer) &&
          (newContainerId == 0 || newContainer) &&
          (baseObjId == 0 || baseObj) && (referenceId == 0 || reference)) {
        AddObjProperty(&obj, "oldContainer", oldContainer, "ObjectReference");
        AddObjProperty(&obj, "newContainer", newContainer, "ObjectReference");
        AddObjProperty(&obj, "reference", reference, "ObjectReference");
        AddObjProperty(&obj, "baseObj", baseObj, "Form");
        AddObjProperty(&obj, "numItems", itemCount);
        AddObjProperty(&obj, "uniqueID", uniqueID);

        SendEvent("containerChanged", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESDeathEvent* event,
                                       RE::BSTEventSource<RE::TESDeathEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto actorDyingId = event->actorDying ? event->actorDying->GetFormID() : 0;
  auto actorKillerId =
    event->actorKiller ? event->actorKiller->GetFormID() : 0;
  auto isDead = event->dead;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([actorDyingId, actorKillerId,
                                                 isDead] {
    auto obj = JsValue::Object();

    auto actorDying = RE::TESForm::LookupByID<RE::TESObjectREFR>(actorDyingId);
    auto actorKiller =
      RE::TESForm::LookupByID<RE::TESObjectREFR>(actorKillerId);

    if (actorDying && actorKiller) {
      AddObjProperty(&obj, "actorDying", actorDying, "ObjectReference");
      AddObjProperty(&obj, "actorKiller", actorKiller, "ObjectReference");

      isDead ? SendEvent("deathEnd", obj) : SendEvent("deathStart", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESDestructionStageChangedEvent* event,
  RE::BSTEventSource<RE::TESDestructionStageChangedEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto targetId = event->target ? event->target->GetFormID() : 0;
  auto oldStage = event->oldStage;
  auto newStage = event->newStage;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [targetId, oldStage, newStage] {
      auto obj = JsValue::Object();

      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);
      if (target) {
        AddObjProperty(&obj, "target", target, "ObjectReference");
        AddObjProperty(&obj, "oldStage", oldStage);
        AddObjProperty(&obj, "newStage", newStage);

        SendEvent("destructionStageChanged", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESEnterBleedoutEvent* event,
  RE::BSTEventSource<RE::TESEnterBleedoutEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto actorId = event->actor ? event->actor->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([actorId] {
    auto obj = JsValue::Object();

    auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);
    if (actor) {
      AddObjProperty(&obj, "actor", actor, "Actor");
      SendEvent("enterBleedout", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESEquipEvent* event,
                                       RE::BSTEventSource<RE::TESEquipEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto actorId = event->actor ? event->actor->GetFormID() : 0;
  auto baseObjectId = event->baseObject;
  auto originalRefrId = event->originalRefr;
  auto uniqueId = event->uniqueID;
  auto equipped = event->equipped;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [actorId, baseObjectId, originalRefrId, uniqueId, equipped] {
      auto obj = JsValue::Object();

      auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);
      auto baseObjForm = RE::TESForm::LookupByID<RE::TESForm>(baseObjectId);
      auto originalRefrForm =
        RE::TESForm::LookupByID<RE::TESForm>(originalRefrId);

      if (actor && baseObjForm && originalRefrForm) {
        AddObjProperty(&obj, "actor", actor, "ObjectReference");
        AddObjProperty(&obj, "baseObj", baseObjForm, "Form");
        AddObjProperty(&obj, "originalRefr", originalRefrForm,
                       "ObjectReference");
        AddObjProperty(&obj, "uniqueId", uniqueId);
        equipped ? SendEvent("equip", obj) : SendEvent("unequip", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESFastTravelEndEvent* event,
  RE::BSTEventSource<RE::TESFastTravelEndEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto travelTimeGameHours = event->travelTimeGameHours;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([travelTimeGameHours] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "travelTimeGameHours", travelTimeGameHours);

    SendEvent("fastTravelEnd", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESFurnitureEvent* event,
  RE::BSTEventSource<RE::TESFurnitureEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto actorId = event->actor ? event->actor->GetFormID() : 0;
  auto targetId =
    event->targetFurniture ? event->targetFurniture->GetFormID() : 0;
  auto type = event->type;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([actorId, targetId, type] {
    auto obj = JsValue::Object();

    auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);
    auto targetFurniture =
      RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);

    if (actor && targetFurniture) {
      AddObjProperty(&obj, "actor", actor, "ObjectReference");
      AddObjProperty(&obj, "target", targetFurniture, "ObjectReference");

      if (type == RE::TESFurnitureEvent::FurnitureEventType::kExit) {
        SendEvent("furnitureExit", obj);
      } else if (type == RE::TESFurnitureEvent::FurnitureEventType::kEnter) {
        SendEvent("furnitureEnter", obj);
      }
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESGrabReleaseEvent* event,
  RE::BSTEventSource<RE::TESGrabReleaseEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto refId = event->ref ? event->ref->GetFormID() : 0;
  auto isGrabbed = event->grabbed;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([refId, isGrabbed] {
    auto obj = JsValue::Object();

    auto refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(refId);
    if (refr) {
      AddObjProperty(&obj, "refr", refr, "ObjectReference");
      AddObjProperty(&obj, "isGrabbed", isGrabbed);

      SendEvent("grabRelease", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESHitEvent* event,
                                       RE::BSTEventSource<RE::TESHitEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto causeId = event->cause ? event->cause->GetFormID() : 0;
  auto targetId = event->target ? event->target->GetFormID() : 0;
  auto sourceId = event->source;
  auto projectileId = event->projectile;
  auto flags = event->flags;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [causeId, targetId, sourceId, projectileId, flags] {
      auto obj = JsValue::Object();

      auto cause = RE::TESForm::LookupByID<RE::TESObjectREFR>(causeId);
      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);
      auto sourceForm = RE::TESForm::LookupByID(sourceId);
      auto projectileForm = RE::TESForm::LookupByID(projectileId);

      if (cause && target) {
        // TODO(#336): drop old name "agressor" on next major release of SP
        // Again: Until we release 3.0.0 we do not remove this line.
        AddObjProperty(&obj, "agressor", cause, "ObjectReference");
        AddObjProperty(&obj, "aggressor", cause, "ObjectReference");
        AddObjProperty(&obj, "target", target, "ObjectReference");
        AddObjProperty(&obj, "source", sourceForm, "Form");
        AddObjProperty(&obj, "projectile", projectileForm, "Form");
        AddObjProperty(&obj, "isPowerAttack",
                       flags.any(RE::TESHitEvent::Flag::kPowerAttack));
        AddObjProperty(&obj, "isSneakAttack",
                       flags.any(RE::TESHitEvent::Flag::kSneakAttack));
        AddObjProperty(&obj, "isBashAttack",
                       flags.any(RE::TESHitEvent::Flag::kBashAttack));
        AddObjProperty(&obj, "isHitBlocked",
                       flags.any(RE::TESHitEvent::Flag::kHitBlocked));

        SendEvent("hit", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESInitScriptEvent* event,
  RE::BSTEventSource<RE::TESInitScriptEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto objectId =
    event->objectInitialized ? event->objectInitialized->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([objectId] {
    auto obj = JsValue::Object();

    auto object = RE::TESForm::LookupByID<RE::TESObjectREFR>(objectId);
    if (object) {
      AddObjProperty(&obj, "initializedObject", object, "ObjectReference");

      SendEvent("scriptInit", obj);
    }
  });

  return EventResult::kContinue;
}

// TODO: Look into LoadGame event
EventResult EventHandler::ProcessEvent(
  const RE::TESLoadGameEvent*, RE::BSTEventSource<RE::TESLoadGameEvent>*)
{
  SkyrimPlatform::GetSingleton()->AddUpdateTask([] { SendEvent("loadGame"); });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESLockChangedEvent* event,
  RE::BSTEventSource<RE::TESLockChangedEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto objectId = event->lockedObject ? event->lockedObject->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([objectId] {
    auto obj = JsValue::Object();

    auto object = RE::TESForm::LookupByID<RE::TESObjectREFR>(objectId);
    if (object) {
      AddObjProperty(&obj, "lockedObject", object, "ObjectReference");

      SendEvent("lockChanged", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESMagicEffectApplyEvent* event,
  RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto casterId = event->caster ? event->caster->GetFormID() : 0;
  auto targetId = event->target ? event->target->GetFormID() : 0;
  auto effectId = event->magicEffect;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [casterId, targetId, effectId] {
      auto obj = JsValue::Object();

      auto effect = RE::TESForm::LookupByID<RE::EffectSetting>(effectId);
      auto caster = RE::TESForm::LookupByID<RE::TESObjectREFR>(casterId);
      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);

      if (effect && caster && target) {
        AddObjProperty(&obj, "effect", effect, "MagicEffect");
        AddObjProperty(&obj, "caster", caster, "ObjectReference");
        AddObjProperty(&obj, "target", target, "ObjectReference");

        SendEvent("magicEffectApply", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESMagicWardHitEvent* event,
  RE::BSTEventSource<RE::TESMagicWardHitEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto casterId = event->caster ? event->caster->GetFormID() : 0;
  auto targetId = event->target ? event->target->GetFormID() : 0;
  auto spellId = event->spell;
  auto status = to_underlying(event->status);

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [casterId, targetId, spellId, status] {
      auto obj = JsValue::Object();

      auto spell = RE::TESForm::LookupByID<RE::SpellItem>(spellId);
      auto caster = RE::TESForm::LookupByID<RE::TESObjectREFR>(casterId);
      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);

      if (caster && target && spell) {
        AddObjProperty(&obj, "caster", caster, "ObjectReference");
        AddObjProperty(&obj, "target", target, "ObjectReference");
        AddObjProperty(&obj, "spell", spell, "Spell");
        AddObjProperty(&obj, "status", status);

        SendEvent("wardHit", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESMoveAttachDetachEvent* event,
  RE::BSTEventSource<RE::TESMoveAttachDetachEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto movedRefId = event->movedRef ? event->movedRef->GetFormID() : 0;
  auto isCellAttached = event->isCellAttached;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([movedRefId, isCellAttached] {
    auto obj = JsValue::Object();

    auto movedRef = RE::TESForm::LookupByID<RE::TESObjectREFR>(movedRefId);

    if (movedRef) {
      AddObjProperty(&obj, "movedRef", movedRef, "ObjectReference");
      AddObjProperty(&obj, "isCellAttached", isCellAttached);

      SendEvent("moveAttachDetach", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESObjectLoadedEvent* event,
  RE::BSTEventSource<RE::TESObjectLoadedEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto formId = event->formID;
  auto loaded = event->loaded;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([formId, loaded] {
    auto obj = JsValue::Object();

    auto object = RE::TESForm::LookupByID(formId);

    AddObjProperty(&obj, "object", object, "Form");
    AddObjProperty(&obj, "isLoaded", loaded);

    SendEvent("objectLoaded", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESObjectREFRTranslationEvent* event,
  RE::BSTEventSource<RE::TESObjectREFRTranslationEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto formId = event->refr ? event->refr.get()->GetFormID() : 0;
  auto type = event->type;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([formId, type] {
    auto obj = JsValue::Object();

    auto reference = RE::TESForm::LookupByID(formId);
    AddObjProperty(&obj, "reference", reference, "ObjectReference");

    switch (type) {
      case RE::TESObjectREFRTranslationEvent::EventType::kFailed: {
        SendEvent("translationFailed", obj);
        break;
      }
      case RE::TESObjectREFRTranslationEvent::EventType::kAlmostCompleted: {
        SendEvent("translationAlmostCompleted", obj);
        break;
      }
      case RE::TESObjectREFRTranslationEvent::EventType::kCompleted: {
        SendEvent("translationCompleted", obj);
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
  if (!event) {
    return EventResult::kContinue;
  }

  auto activeRefFormId =
    event->activeRef ? event->activeRef.get()->GetFormID() : 0;
  auto refFormId = event->ref ? event->ref.get()->GetFormID() : 0;
  bool isOpened = event->opened;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [activeRefFormId, refFormId, isOpened] {
      auto obj = JsValue::Object();

      auto cause = RE::TESForm::LookupByID(activeRefFormId);
      auto target = RE::TESForm::LookupByID(refFormId);
      AddObjProperty(&obj, "cause", cause, "ObjectReference");
      AddObjProperty(&obj, "target", target, "ObjectReference");

      if (isOpened) {
        SendEvent("open", obj);
      } else {
        SendEvent("close", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESPackageEvent* event, RE::BSTEventSource<RE::TESPackageEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto actorFormId = event->actor ? event->actor.get()->GetFormID() : 0;
  auto packageFormId = event->package;
  auto eventType = event->type;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [actorFormId, packageFormId, eventType] {
      auto obj = JsValue::Object();

      auto actor = RE::TESForm::LookupByID(actorFormId);
      auto package = RE::TESForm::LookupByID(packageFormId);
      AddObjProperty(&obj, "actor", actor, "ObjectReference");
      AddObjProperty(&obj, "package", package, "Package");

      switch (eventType) {
        case RE::TESPackageEvent::EventType::kStart: {
          SendEvent("packageStart", obj);
          break;
        }
        case RE::TESPackageEvent::EventType::kChange: {
          SendEvent("packageChange", obj);
          break;
        }
        case RE::TESPackageEvent::EventType::kEnd: {
          SendEvent("packageEnd", obj);
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
  if (!event) {
    return EventResult::kContinue;
  }

  auto perkId = event->perkId;
  auto causeFormId = event->cause ? event->cause.get()->GetFormID() : 0;
  auto targetFormId = event->target ? event->target.get()->GetFormID() : 0;
  auto flag = event->flag;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [perkId, causeFormId, targetFormId, flag] {
      auto obj = JsValue::Object();

      auto perk = RE::TESForm::LookupByID(perkId);
      auto cause = RE::TESForm::LookupByID(causeFormId);
      auto target = RE::TESForm::LookupByID(targetFormId);

      AddObjProperty(&obj, "cause", cause, "ObjectReference");
      AddObjProperty(&obj, "target", target, "ObjectReference");
      AddObjProperty(&obj, "perk", perk, "Perk");
      AddObjProperty(&obj, "flag", flag);

      SendEvent("perkEntryRun", obj);
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESPlayerBowShotEvent* event,
  RE::BSTEventSource<RE::TESPlayerBowShotEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto weaponId = event->weaponId;
  auto ammoId = event->ammoId;
  auto power = event->power;
  auto isSunGazing = event->isSunGazing;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [weaponId, ammoId, power, isSunGazing] {
      auto obj = JsValue::Object();

      auto weapon = RE::TESForm::LookupByID(weaponId);
      auto ammo = RE::TESForm::LookupByID(ammoId);

      AddObjProperty(&obj, "weapon", weapon, "Weapon");
      AddObjProperty(&obj, "ammo", ammo, "Ammo");
      AddObjProperty(&obj, "power", power);
      AddObjProperty(&obj, "target", isSunGazing);

      SendEvent("playerBowShot", obj);
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESQuestInitEvent* event,
  RE::BSTEventSource<RE::TESQuestInitEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto questId = event->questId;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([questId] {
    auto obj = JsValue::Object();

    auto quest = RE::TESForm::LookupByID(questId);

    AddObjProperty(&obj, "quest", quest, "Quest");
    SendEvent("questInit", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESQuestStageEvent* event,
  RE::BSTEventSource<RE::TESQuestStageEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto questId = event->questId;
  auto stage = event->stage;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([questId, stage] {
    auto obj = JsValue::Object();

    auto quest = RE::TESForm::LookupByID(questId);

    AddObjProperty(&obj, "quest", quest, "Quest");
    AddObjProperty(&obj, "stage", stage);

    SendEvent("questStage", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESQuestStartStopEvent* event,
  RE::BSTEventSource<RE::TESQuestStartStopEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto questId = event->questId;
  auto isStarted = event->isStarted;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([questId, isStarted] {
    auto obj = JsValue::Object();

    auto quest = RE::TESForm::LookupByID(questId);

    AddObjProperty(&obj, "quest", quest, "Quest");

    if (isStarted) {
      SendEvent("questStart", obj);
    } else {
      SendEvent("questStop", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESResetEvent* event,
                                       RE::BSTEventSource<RE::TESResetEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto objectId = event->object ? event->object.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([objectId] {
    auto obj = JsValue::Object();

    auto object = RE::TESForm::LookupByID(objectId);

    AddObjProperty(&obj, "object", object, "ObjectReference");
    SendEvent("reset", obj);
  });

  return EventResult::kStop;
}

EventResult EventHandler::ProcessEvent(const RE::TESSellEvent* event,
                                       RE::BSTEventSource<RE::TESSellEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto sellerId = event->seller ? event->seller.get()->GetFormID() : 0;
  auto targetId = event->target ? event->target.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([sellerId, targetId] {
    auto obj = JsValue::Object();

    auto seller = RE::TESForm::LookupByID(sellerId);
    auto target = RE::TESForm::LookupByID(targetId);

    AddObjProperty(&obj, "seller", seller, "ObjectReference");
    AddObjProperty(&obj, "target", target, "ObjectReference");

    SendEvent("sell", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSceneActionEvent* event,
  RE::BSTEventSource<RE::TESSceneActionEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto sceneId = event->sceneId;
  auto questId = event->questId;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [sceneId, questId, actorAliasId = event->actorAliasId,
     actionIndex = event->actionIndex] {
      auto obj = JsValue::Object();

      auto scene = RE::TESForm::LookupByID<RE::BGSScene>(sceneId);
      auto quest = RE::TESForm::LookupByID<RE::TESQuest>(questId);

      AddObjProperty(&obj, "actorAliasId", actorAliasId);
      AddObjProperty(&obj, "actionIndex", actionIndex);
      AddObjProperty(&obj, "scene", scene, "Scene");
      AddObjProperty(&obj, "quest", quest, "Quest");

      SendEvent("sceneAction", obj);
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSleepStartEvent* event,
  RE::BSTEventSource<RE::TESSleepStartEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto startTime = event->sleepStartTime;
  auto desiredStopTime = event->desiredSleepEndTime;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([startTime, desiredStopTime] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "startTime", startTime);
    AddObjProperty(&obj, "desiredStopTime", desiredStopTime);

    SendEvent("sleepStart", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSleepStopEvent* event,
  RE::BSTEventSource<RE::TESSleepStopEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto isInterrupted = event->isInterrupted;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([isInterrupted] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "isInterrupted", isInterrupted);
    SendEvent("sleepStop", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSpellCastEvent* event,
  RE::BSTEventSource<RE::TESSpellCastEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto casterId = event->caster ? event->caster->GetFormID() : 0;
  auto spellId = event->spell;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([casterId, spellId] {
    auto obj = JsValue::Object();

    auto spell = RE::TESForm::LookupByID(spellId);

    if (casterId != 0) {
      auto caster = RE::TESForm::LookupByID<RE::Actor>(casterId);
      if (caster) {
        AddObjProperty(&obj, "caster", caster, "ObjectReference");
        AddObjProperty(&obj, "spell", spell, "Spell");

        SendEvent("spellCast", obj);
      }
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSwitchRaceCompleteEvent* event,
  RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto subjectId = event->subject ? event->subject->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([subjectId] {
    auto obj = JsValue::Object();

    if (subjectId != 0) {
      auto subject = RE::TESForm::LookupByID<RE::Actor>(subjectId);
      if (subject) {
        AddObjProperty(&obj, "subject", subject, "ObjectReference");
        SendEvent("switchRaceComplete", obj);
      }
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESTrackedStatsEvent* event,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto statName = event->stat;
  auto value = event->value;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([statName, value] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "statName", statName);
    AddObjProperty(&obj, "newValue", value);

    SendEvent("trackedStats", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESTriggerEnterEvent* event,
  RE::BSTEventSource<RE::TESTriggerEnterEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto causeId = event->caster ? event->caster->GetFormID() : 0;
  auto targetId = event->target ? event->target->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([causeId, targetId] {
    auto obj = JsValue::Object();

    auto cause = RE::TESForm::LookupByID(causeId);
    auto target = RE::TESForm::LookupByID(targetId);

    if (cause && target) {
      AddObjProperty(&obj, "cause", cause, "ObjectReference");
      AddObjProperty(&obj, "target", target, "ObjectReference");

      SendEvent("triggerEnter", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESTriggerEvent* event, RE::BSTEventSource<RE::TESTriggerEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto causeId = event->caster ? event->caster->GetFormID() : 0;
  auto targetId = event->target ? event->target->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([causeId, targetId] {
    auto obj = JsValue::Object();

    auto cause = RE::TESForm::LookupByID(causeId);
    auto target = RE::TESForm::LookupByID(targetId);

    if (cause && target) {
      AddObjProperty(&obj, "cause", cause, "ObjectReference");
      AddObjProperty(&obj, "target", target, "ObjectReference");

      SendEvent("trigger", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESTriggerLeaveEvent* event,
  RE::BSTEventSource<RE::TESTriggerLeaveEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto causeId = event->caster ? event->caster->GetFormID() : 0;
  auto targetId = event->target ? event->target->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([causeId, targetId] {
    auto obj = JsValue::Object();

    auto cause = RE::TESForm::LookupByID(causeId);
    auto target = RE::TESForm::LookupByID(targetId);

    if (cause && target) {
      AddObjProperty(&obj, "cause", cause, "ObjectReference");
      AddObjProperty(&obj, "target", target, "ObjectReference");

      SendEvent("triggerLeave", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESUniqueIDChangeEvent* event,
  RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "oldBaseID", e->oldBaseID);
    AddObjProperty(&obj, "newBaseID", e->newBaseID);
    AddObjProperty(&obj, "oldUniqueID", e->oldUniqueID);
    AddObjProperty(&obj, "newUniqueID", e->newUniqueID);

    SendEvent("uniqueIdChange", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESWaitStartEvent* event,
  RE::BSTEventSource<RE::TESWaitStartEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto startTime = event->waitStartTime;
  auto desiredStopTime = event->desiredWaitEndTime;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([startTime, desiredStopTime] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "startTime", startTime);
    AddObjProperty(&obj, "desiredStopTime", desiredStopTime);

    SendEvent("waitStart", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESWaitStopEvent* event, RE::BSTEventSource<RE::TESWaitStopEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto isInterrupted = event->interrupted;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([isInterrupted] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "isInterrupted", isInterrupted);

    SendEvent("waitStop", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const SKSE::ActionEvent* event,
                                       RE::BSTEventSource<SKSE::ActionEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto actorId = event->actor ? event->actor->GetFormID() : 0;
  auto sourceId = event->sourceForm ? event->sourceForm->GetFormID() : 0;
  auto slot = event->slot ? to_underlying(event->slot.get()) : 0;
  auto type = event->type ? to_underlying(event->type.get()) : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [actorId, sourceId, slot, type] {
      auto obj = JsValue::Object();

      auto actor = RE::TESForm::LookupByID(actorId);
      if (actor) {
        AddObjProperty(&obj, "actor", actor, "Actor");
      }

      auto source = RE::TESForm::LookupByID(sourceId);
      if (source) {
        AddObjProperty(&obj, "source", source, "Form");
      }

      AddObjProperty(&obj, "slot", slot);

      switch (static_cast<SKSE::ActionEvent::Type>(type)) {
        case SKSE::ActionEvent::Type::kWeaponSwing: {
          SendEvent("actionWeaponSwing", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kBeginDraw: {
          SendEvent("actionBeginDraw", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kEndDraw: {
          SendEvent("actionEndDraw", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kBowDraw: {
          SendEvent("actionBowDraw", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kBowRelease: {
          SendEvent("actionBowRelease", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kBeginSheathe: {
          SendEvent("actionBeginSheathe", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kEndSheathe: {
          SendEvent("actionEndSheathe", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kSpellCast: {
          SendEvent("actionSpellCast", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kSpellFire: {
          SendEvent("actionSpellFire", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kVoiceCast: {
          SendEvent("actionVoiceCast", obj);
          break;
        }
        case SKSE::ActionEvent::Type::kVoiceFire: {
          SendEvent("actionVoiceFire", obj);
          break;
        }
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const SKSE::CameraEvent* event,
                                       RE::BSTEventSource<SKSE::CameraEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto oldStateId = to_underlying(event->oldState->id);
  auto newStateId = to_underlying(event->newState->id);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([oldStateId, newStateId] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "oldStateId", oldStateId);
    AddObjProperty(&obj, "newStateId", newStateId);

    SendEvent("cameraStateChanged", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const SKSE::CrosshairRefEvent* event,
  RE::BSTEventSource<SKSE::CrosshairRefEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto referenceId =
    event->crosshairRef ? event->crosshairRef->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([referenceId] {
    auto obj = JsValue::Object();

    auto reference = RE::TESForm::LookupByID(referenceId);

    if (reference) {
      AddObjProperty(&obj, "reference", reference, "ObjectReference");
      SendEvent("crosshairRefChanged", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const SKSE::NiNodeUpdateEvent* event,
  RE::BSTEventSource<SKSE::NiNodeUpdateEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto referenceId = event->reference ? event->reference->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([referenceId] {
    auto obj = JsValue::Object();

    auto reference = RE::TESForm::LookupByID(referenceId);

    if (reference) {
      AddObjProperty(&obj, "reference", reference, "ObjectReference");
      SendEvent("niNodeUpdate", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const SKSE::ModCallbackEvent* event,
  RE::BSTEventSource<SKSE::ModCallbackEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto senderId = event->sender ? event->sender->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [senderId, eventName = event->eventName, strArg = event->strArg,
     numArg = event->numArg] {
      auto obj = JsValue::Object();

      auto sender = RE::TESForm::LookupByID(senderId);

      AddObjProperty(&obj, "sender", sender, "Form");
      AddObjProperty(&obj, "eventName", eventName);
      AddObjProperty(&obj, "strArg", strArg);
      AddObjProperty(&obj, "numArg", numArg);

      SendEvent("modEvent", obj);
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

  auto menuName = event->menuName;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [menuName, opening = event->opening] {
      auto obj = JsValue::Object();

      AddObjProperty(&obj, "name", menuName);

      if (opening) {
        SendEvent("menuOpen", obj);
      } else {
        SendEvent("menuClose", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(RE::InputEvent* const* event,
                                       RE::BSTEventSource<RE::InputEvent*>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    for (auto eventItem = *e; eventItem; eventItem = eventItem->next) {
      if (!eventItem) {
        return;
      }

      auto device = to_underlying(eventItem->device.get());

      switch (eventItem->eventType.get()) {
        case RE::INPUT_EVENT_TYPE::kButton: {
          auto buttonEvent = static_cast<RE::ButtonEvent*>(eventItem);
          auto obj = JsValue::Object();

          AddObjProperty(&obj, "device", device);
          AddObjProperty(&obj, "code", buttonEvent->idCode);
          AddObjProperty(&obj, "userEventName", buttonEvent->userEvent);
          AddObjProperty(&obj, "value", buttonEvent->value);
          AddObjProperty(&obj, "heldDuration", buttonEvent->heldDownSecs);
          AddObjProperty(&obj, "isPressed", buttonEvent->IsPressed());
          AddObjProperty(&obj, "isUp", buttonEvent->IsUp());
          AddObjProperty(&obj, "isDown", buttonEvent->IsDown());
          AddObjProperty(&obj, "isHeld", buttonEvent->IsHeld());
          AddObjProperty(&obj, "isRepeating", buttonEvent->IsRepeating());

          SendEvent("buttonEvent", obj);
          break;
        }
        case RE::INPUT_EVENT_TYPE::kMouseMove: {
          auto mouseEvent = static_cast<RE::MouseMoveEvent*>(eventItem);
          auto obj = JsValue::Object();

          AddObjProperty(&obj, "device", device);
          AddObjProperty(&obj, "code", mouseEvent->idCode);
          AddObjProperty(&obj, "userEventName", mouseEvent->userEvent);
          AddObjProperty(&obj, "inputX", mouseEvent->mouseInputX);
          AddObjProperty(&obj, "inputY", mouseEvent->mouseInputY);

          SendEvent("mouseMove", obj);
          break;
        }
        case RE::INPUT_EVENT_TYPE::kDeviceConnect: {
          auto deviceConnectEvent =
            static_cast<RE::DeviceConnectEvent*>(eventItem);
          auto obj = JsValue::Object();

          AddObjProperty(&obj, "device", device);
          AddObjProperty(&obj, "isConnected", deviceConnectEvent->connected);

          SendEvent("deviceConnect", obj);
          break;
        }
        case RE::INPUT_EVENT_TYPE::kThumbstick: {
          auto thumbstickEvent = static_cast<RE::ThumbstickEvent*>(eventItem);
          auto obj = JsValue::Object();

          AddObjProperty(&obj, "device", device);
          AddObjProperty(&obj, "code", thumbstickEvent->idCode);
          AddObjProperty(&obj, "userEventName", thumbstickEvent->userEvent);
          AddObjProperty(&obj, "inputX", thumbstickEvent->xValue);
          AddObjProperty(&obj, "inputY", thumbstickEvent->yValue);
          AddObjProperty(&obj, "isLeft", thumbstickEvent->IsLeft());
          AddObjProperty(&obj, "isRight", thumbstickEvent->IsRight());

          SendEvent("thumbstickEvent", obj);
          break;
        }
        case RE::INPUT_EVENT_TYPE::kKinect: {
          auto kinectEvent = static_cast<RE::KinectEvent*>(eventItem);
          auto obj = JsValue::Object();

          AddObjProperty(&obj, "device", device);
          AddObjProperty(&obj, "code", kinectEvent->idCode);
          AddObjProperty(&obj, "userEventName", kinectEvent->userEvent);
          AddObjProperty(&obj, "heard", kinectEvent->heard);

          SendEvent("kinectEvent", obj);
          break;
        }
      }
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

  auto tag = event->tag;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([tag] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "tag", tag);

    SendEvent("footstep", obj);
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

  auto type = event->type ? to_underlying(event->type.get()) : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([type] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "eventType", type);

    SendEvent("positionPlayer", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::ActorKill::Event* event, RE::BSTEventSource<RE::ActorKill::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto killerId = event->killer ? event->killer->GetFormID() : 0;
  auto victimId = event->victim ? event->victim->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([killerId, victimId] {
    auto obj = JsValue::Object();

    auto killer = RE::TESForm::LookupByID<RE::Actor>(killerId);
    auto victim = RE::TESForm::LookupByID<RE::Actor>(victimId);

    if (killer) {
      AddObjProperty(&obj, "killer", killer, "Actor");
    }

    if (victim) {
      AddObjProperty(&obj, "victim", victim, "Actor");
    }

    SendEvent("actorKill", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::BooksRead::Event* event, RE::BSTEventSource<RE::BooksRead::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto bookId = event->book ? event->book->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([bookId] {
    auto obj = JsValue::Object();

    auto book = RE::TESForm::LookupByID<RE::TESObjectBOOK>(bookId);

    if (book) {
      AddObjProperty(&obj, "book", book, "Book");
    }

    SendEvent("bookRead", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::CriticalHit::Event* event,
  RE::BSTEventSource<RE::CriticalHit::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto aggressorId = event->aggressor ? event->aggressor->GetFormID() : 0;
  auto weaponId = event->weapon ? event->weapon->GetFormID() : 0;
  auto sneakHit = event->sneakHit;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [aggressorId, weaponId, sneakHit] {
      auto obj = JsValue::Object();

      auto aggressor = RE::TESForm::LookupByID(aggressorId);
      auto weapon = RE::TESForm::LookupByID(weaponId);

      AddObjProperty(&obj, "aggressor", aggressor, "ObjectReference");
      AddObjProperty(&obj, "weapon", weapon, "Weapon");
      AddObjProperty(&obj, "isSneakHit", sneakHit);

      SendEvent("criticalHit", obj);
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::DisarmedEvent::Event* event,
  RE::BSTEventSource<RE::DisarmedEvent::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto sourceId = event->source ? event->source->GetFormID() : 0;
  auto targetId = event->target ? event->target->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([sourceId, targetId] {
    auto obj = JsValue::Object();

    auto source = RE::TESForm::LookupByID(sourceId);
    auto target = RE::TESForm::LookupByID(targetId);

    AddObjProperty(&obj, "source", source, "Actor");
    AddObjProperty(&obj, "target", target, "Actor");

    SendEvent("disarmedEvent", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::DragonSoulsGained::Event* event,
  RE::BSTEventSource<RE::DragonSoulsGained::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "souls", e->souls);

    SendEvent("dragonSoulsGained", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::ItemHarvested::Event* event,
  RE::BSTEventSource<RE::ItemHarvested::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto produceItemId =
    event->produceItem ? event->produceItem->GetFormID() : 0;
  auto harvesterId = event->harvester ? event->harvester->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([produceItemId, harvesterId] {
    auto obj = JsValue::Object();

    auto produceItem = RE::TESForm::LookupByID(produceItemId);
    auto harvester = RE::TESForm::LookupByID(harvesterId);

    AddObjProperty(&obj, "produceItem", produceItem, "Form");
    AddObjProperty(&obj, "harvester", harvester, "Actor");

    SendEvent("itemHarvested", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::LevelIncrease::Event* event,
  RE::BSTEventSource<RE::LevelIncrease::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto playerFormId = event->player ? event->player->GetFormID() : 0;
  auto newLevel = event->newLevel;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([playerFormId, newLevel] {
    auto obj = JsValue::Object();

    auto player = RE::TESForm::LookupByID(playerFormId);

    if (player) {
      AddObjProperty(&obj, "player", player, "Actor");
      AddObjProperty(&obj, "newLevel", newLevel);

      SendEvent("levelIncrease", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::LocationDiscovery::Event* event,
  RE::BSTEventSource<RE::LocationDiscovery::Event>*)
{
  if (!event || event->mapMarkerData == nullptr) {
    return EventResult::kContinue;
  }

  auto worldspaceId = event->worldspaceID;
  auto markerType = event->mapMarkerData->type.get();
  auto markerFullName = event->mapMarkerData->locationName.fullName;
  auto markerFlags = event->mapMarkerData->flags;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [worldspaceId, markerType, markerFullName, markerFlags] {
      auto obj = JsValue::Object();

      auto type = to_underlying(markerType);

      AddObjProperty(&obj, "worldSpaceId", worldspaceId);
      AddObjProperty(&obj, "name", markerFullName);
      AddObjProperty(&obj, "markerType", type);
      AddObjProperty(&obj, "isVisible",
                     markerFlags.any(RE::MapMarkerData::Flag::kVisible));
      AddObjProperty(&obj, "canTravelTo",
                     markerFlags.any(RE::MapMarkerData::Flag::kCanTravelTo));
      AddObjProperty(&obj, "isShowAllHidden",
                     markerFlags.any(RE::MapMarkerData::Flag::kShowAllHidden));

      SendEvent("locationDiscovery", obj);
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::ShoutAttack::Event* event,
  RE::BSTEventSource<RE::ShoutAttack::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto shoutFormId = event->shout ? event->shout->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([shoutFormId] {
    auto obj = JsValue::Object();

    auto shout = RE::TESForm::LookupByID(shoutFormId);

    if (shout) {
      AddObjProperty(&obj, "shout", shout, "Shout");
      SendEvent("shoutAttack", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::SkillIncrease::Event* event,
  RE::BSTEventSource<RE::SkillIncrease::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto playerFormId = event->player ? event->player->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [playerFormId, actorValue = event->actorValue] {
      auto obj = JsValue::Object();

      auto player = RE::TESForm::LookupByID(playerFormId);

      if (player) {
        AddObjProperty(&obj, "player", player, "Actor");
        AddObjProperty(&obj, "actorValue", to_underlying(actorValue));

        SendEvent("skillIncrease", obj);
      }
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::SoulsTrapped::Event* event,
  RE::BSTEventSource<RE::SoulsTrapped::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto trapperFormId = event->trapper ? event->trapper->GetFormID() : 0;
  auto targetFormId = event->target ? event->target->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([trapperFormId, targetFormId] {
    auto obj = JsValue::Object();

    auto trapper = RE::TESForm::LookupByID(trapperFormId);
    auto target = RE::TESForm::LookupByID(targetFormId);

    if (trapper && target) {
      AddObjProperty(&obj, "trapper", trapper, "Actor");
      AddObjProperty(&obj, "target", target, "Actor");

      SendEvent("soulsTrapped", obj);
    }
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::SpellsLearned::Event* event,
  RE::BSTEventSource<RE::SpellsLearned::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto spellFormId = event->spell ? event->spell->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([spellFormId] {
    auto obj = JsValue::Object();

    auto spell = RE::TESForm::LookupByID(spellFormId);

    if (spell) {
      AddObjProperty(&obj, "spell", spell, "Spell");
      SendEvent("spellsLearned", obj);
    }
  });

  return EventResult::kContinue;
}
