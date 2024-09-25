#include "EventHandler.h"
#include "EventManager.h"
#include "EventsApi.h"
#include "JsUtils.h"
#include "SkyrimPlatform.h"

namespace {
inline void SendEvent(const char* eventName)
{
  EventsApi::SendEvent(eventName, {});
}

inline void SendEvent(const char* eventName, const Napi::Object& obj)
{
  EventsApi::SendEvent(eventName, { obj });
}
}

void EventHandler::SendSimpleEventOnUpdate(const char* eventName)
{
  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [eventName](Napi::Env) { SendEvent(eventName); });
}

void EventHandler::SendSimpleEventOnTick(const char* eventName)
{
  SkyrimPlatform::GetSingleton()->AddTickTask(
    [eventName](Napi::Env) { SendEvent(eventName); });
}

void EventHandler::SendEventOnUpdate(const char* eventName,
                                     const Napi::Object& obj)
{
  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [eventName, obj](Napi::Env) { SendEvent(eventName, obj); });
}

void EventHandler::SendEventOnTick(const char* eventName,
                                   const Napi::Object& obj)
{
  SkyrimPlatform::GetSingleton()->AddTickTask(
    [eventName, obj](Napi::Env) { SendEvent(eventName, obj); });
}

void EventHandler::SendEventConsoleMsg(const char* msg)
{
  std::string msgStr = msg;

  SkyrimPlatform::GetSingleton()->AddTickTask([msgStr](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    AddObjProperty(&obj, "message", msgStr.data());

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

  uint32_t objectActivatedId = event->objectActivated.get()
    ? event->objectActivated.get()->GetFormID()
    : 0;

  uint32_t actionRefId =
    event->actionRef.get() ? event->actionRef.get()->GetFormID() : 0;

  bool isCrimeToActivate = event->objectActivated.get()->IsCrimeToActivate();

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [objectActivatedId, actionRefId, isCrimeToActivate](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto objectActivated =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(objectActivatedId);
      auto actionRef = RE::TESForm::LookupByID<RE::TESObjectREFR>(actionRefId);

      if (!objectActivated && objectActivatedId != 0) {
        return;
      }

      if (!actionRef && actionRefId != 0) {
        return;
      }

      AddObjProperty(&obj, "target", objectActivated, "ObjectReference");
      AddObjProperty(&obj, "caster", actionRef, "ObjectReference");
      AddObjProperty(&obj, "isCrimeToActivate", isCrimeToActivate);

      SendEvent("activate", obj);
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESActiveEffectApplyRemoveEvent* event,
  RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  struct EffectData
  {
    uint32_t baseMagicEffectId = 0;
    uint32_t casterRefrId = 0;
    uint32_t targetRefrId = 0;
    bool isApplied = false;
  };

  EffectData effectData;
  effectData.casterRefrId =
    event->caster.get() ? event->caster.get()->GetFormID() : 0;
  effectData.targetRefrId =
    event->target.get() ? event->target.get()->GetFormID() : 0;
  effectData.isApplied = event->isApplied;

  RE::Actor* targetActor =
    event->target.get() ? event->target.get()->As<RE::Actor>() : nullptr;

  if (targetActor) {
    for (RE::ActiveEffect* eff : *targetActor->GetActiveEffectList()) {
      if (eff->usUniqueID == event->activeEffectUniqueID) {
        auto baseMagicEffect = eff->GetBaseObject();
        effectData.baseMagicEffectId =
          baseMagicEffect ? baseMagicEffect->formID : 0;
        break;
      }
    }
  }

  if (effectData.baseMagicEffectId == 0) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton()->AddUpdateTask([effectData](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto baseMagicEffect =
      RE::TESForm::LookupByID<RE::EffectSetting>(effectData.baseMagicEffectId);
    auto casterRefr =
      RE::TESForm::LookupByID<RE::TESObjectREFR>(effectData.casterRefrId);
    auto targetRefr =
      RE::TESForm::LookupByID<RE::TESObjectREFR>(effectData.targetRefrId);

    if (!baseMagicEffect) {
      return;
    }

    if (!casterRefr && effectData.casterRefrId != 0) {
      return;
    }

    if (!targetRefr && effectData.targetRefrId != 0) {
      return;
    }

    AddObjProperty(&obj, "effect", baseMagicEffect, "MagicEffect");
    AddObjProperty(&obj, "caster", casterRefr, "ObjectReference");
    AddObjProperty(&obj, "target", targetRefr, "ObjectReference");

    if (effectData.isApplied) {
      SendEvent("effectStart", obj);
    } else {
      SendEvent("effectFinish", obj);
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

  auto actorId = event->actor.get() ? event->actor.get()->GetFormID() : 0;
  auto oldLocId = event->oldLoc ? event->oldLoc->formID : 0;
  auto newLocId = event->newLoc ? event->newLoc->formID : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [actorId, oldLocId, newLocId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);
      auto oldLoc = RE::TESForm::LookupByID<RE::BGSLocation>(oldLocId);
      auto newLoc = RE::TESForm::LookupByID<RE::BGSLocation>(newLocId);

      if (!actor && actorId != 0) {
        return;
      }

      if (!oldLoc && oldLocId != 0) {
        return;
      }

      if (!newLoc && newLocId != 0) {
        return;
      }

      AddObjProperty(&obj, "actor", actor, "Actor");
      AddObjProperty(&obj, "oldLoc", oldLoc, "Location");
      AddObjProperty(&obj, "newLoc", newLoc, "Location");

      SendEvent("locationChanged", obj);
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

  auto refr = event->reference.get();
  if (!refr) {
    return EventResult::kContinue;
  }
  auto refrId = refr->GetFormID();

  bool isAttach = event->attached;
  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [refrId, isAttach](Napi::Env env) {
      auto refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(refrId);
      if (!refr) {
        return;
      }

      auto obj = Napi::Object::New(env);
      AddObjProperty(&obj, "refr", refr, "ObjectReference");
      isAttach ? SendEvent("cellAttach", obj) : SendEvent("cellDetach", obj);
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

  uint32_t cellId = event->cell ? event->cell->formID : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([cellId](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(cellId);

    if (!cell && cellId != 0) {
      return;
    }

    AddObjProperty(&obj, "cell", cell, "Cell");

    SendEvent("cellFullyLoaded", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESCombatEvent* event,
                                       RE::BSTEventSource<RE::TESCombatEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto target = event->targetActor.get();
  auto actor = event->actor.get();

  if (!target || !actor) {
    return EventResult::kContinue;
  }

  auto targetId = target->GetFormID();
  auto actorId = actor->GetFormID();

  bool isCombat = event->newState.any(RE::ACTOR_COMBAT_STATE::kCombat);
  bool isSearching = event->newState.any(RE::ACTOR_COMBAT_STATE::kSearching);

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [targetId, actorId, isCombat, isSearching](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);
      auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);

      if (!target && targetId != 0) {
        return;
      }

      if (!actor && actorId != 0) {
        return;
      }

      AddObjProperty(&obj, "target", target, "ObjectReference");
      AddObjProperty(&obj, "actor", actor, "ObjectReference");
      AddObjProperty(&obj, "isCombat", isCombat);
      AddObjProperty(&obj, "isSearching", isSearching);

      SendEvent("combatState", obj);
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

  auto refr = event->reference.get().get();
  auto refrId = refr ? refr->GetFormID() : 0;

  auto oldContainerId = event->oldContainer;
  auto newContainerId = event->newContainer;
  auto baseObjId = event->baseObj;
  auto itemCount = event->itemCount;
  auto uniqueID = event->uniqueID;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [refrId, oldContainerId, newContainerId, baseObjId, itemCount,
     uniqueID](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto contFormOld =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(oldContainerId);
      auto contFormNew =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(newContainerId);
      auto baseObjForm = RE::TESForm::LookupByID(baseObjId);
      auto reference = RE::TESForm::LookupByID<RE::TESObjectREFR>(refrId);

      if (!reference && refrId != 0) {
        return;
      }

      if (!contFormOld && oldContainerId != 0) {
        return;
      }

      if (!contFormNew && newContainerId != 0) {
        return;
      }

      if (!baseObjForm && baseObjId != 0) {
        return;
      }

      AddObjProperty(&obj, "oldContainer", contFormOld, "ObjectReference");
      AddObjProperty(&obj, "newContainer", contFormNew, "ObjectReference");
      AddObjProperty(&obj, "baseObj", baseObjForm, "Form");
      AddObjProperty(&obj, "numItems", itemCount);
      AddObjProperty(&obj, "uniqueID", uniqueID);
      AddObjProperty(&obj, "reference", reference, "ObjectReference");

      SendEvent("containerChanged", obj);
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESDeathEvent* event,
                                       RE::BSTEventSource<RE::TESDeathEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto actorDyingId =
    event->actorDying.get() ? event->actorDying.get()->GetFormID() : 0;

  auto actorKillerId =
    event->actorKiller.get() ? event->actorKiller.get()->GetFormID() : 0;

  bool dead = event->dead;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [actorDyingId, actorKillerId, dead](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto actorDying = RE::TESForm::LookupByID<RE::Actor>(actorDyingId);

      if (!actorDying && actorDyingId != 0) {
        return;
      }

      auto actorKiller = RE::TESForm::LookupByID<RE::Actor>(actorKillerId);

      if (!actorKiller && actorKillerId != 0) {
        return;
      }

      AddObjProperty(&obj, "actorDying", actorDying, "ObjectReference");
      AddObjProperty(&obj, "actorKiller", actorKiller, "ObjectReference");

      dead ? SendEvent("deathEnd", obj) : SendEvent("deathStart", obj);
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

  auto targetId = event->target.get() ? event->target.get()->GetFormID() : 0;
  auto oldStage = event->oldStage;
  auto newStage = event->newStage;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [targetId, oldStage, newStage](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);

      if (!target && targetId != 0) {
        return;
      }

      AddObjProperty(&obj, "target", target, "ObjectReference");
      AddObjProperty(&obj, "oldStage", oldStage);
      AddObjProperty(&obj, "newStage", newStage);

      SendEvent("destructionStageChanged", obj);
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

  auto actorId = event->actor.get() ? event->actor.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([actorId](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);

    if (!actor && actorId != 0) {
      return;
    }

    AddObjProperty(&obj, "actor", actor, "ObjectReference");

    SendEvent("enterBleedout", obj);
  });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(const RE::TESEquipEvent* event,
                                       RE::BSTEventSource<RE::TESEquipEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto baseObject = event->baseObject;
  auto originalRefr = event->originalRefr;
  auto actorId = event->actor.get() ? event->actor.get()->GetFormID() : 0;
  auto equipped = event->equipped;
  auto uniqueID = event->uniqueID;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [baseObject, originalRefr, actorId, equipped, uniqueID](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto baseObjForm = RE::TESForm::LookupByID(baseObject);

      if (!baseObjForm && baseObject != 0) {
        return;
      }

      auto originalRefrForm =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(originalRefr);

      if (!originalRefrForm && originalRefr != 0) {
        return;
      }

      auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);

      if (!actor && actorId != 0) {
        return;
      }

      AddObjProperty(&obj, "actor", actor, "ObjectReference");
      AddObjProperty(&obj, "baseObj", baseObjForm, "Form");
      AddObjProperty(&obj, "originalRefr", originalRefrForm,
                     "ObjectReference");
      AddObjProperty(&obj, "uniqueId", uniqueID);

      equipped ? SendEvent("equip", obj) : SendEvent("unequip", obj);
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

  auto fastTravelEndHours = event->fastTravelEndHours;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [fastTravelEndHours](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      AddObjProperty(&obj, "travelTimeGameHours", fastTravelEndHours);

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

  auto type = event->type;
  auto actorId = event->actor.get() ? event->actor.get()->GetFormID() : 0;
  auto targetFurnitureId = event->targetFurniture.get()
    ? event->targetFurniture.get()->GetFormID()
    : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [type, actorId, targetFurnitureId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);

      if (!actor && actorId != 0) {
        return;
      }

      auto targetFurniture =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(targetFurnitureId);

      if (!targetFurniture && targetFurnitureId != 0) {
        return;
      }

      AddObjProperty(&obj, "actor", actor, "ObjectReference");
      AddObjProperty(&obj, "target", targetFurniture, "ObjectReference");

      if (type == RE::TESFurnitureEvent::FurnitureEventType::kExit) {
        SendEvent("furnitureExit", obj);
      } else if (type == RE::TESFurnitureEvent::FurnitureEventType::kEnter) {
        SendEvent("furnitureEnter", obj);
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

  auto refId = event->ref.get() ? event->ref.get()->GetFormID() : 0;
  auto grabbed = event->grabbed;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [refId, grabbed](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(refId);

      if (!ref && refId != 0) {
        return;
      }

      AddObjProperty(&obj, "refr", ref, "ObjectReference");
      AddObjProperty(&obj, "isGrabbed", grabbed);

      SendEvent("grabRelease", obj);
    });

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESHitEvent* event,
  RE::BSTEventSource<RE::TESHitEvent>* eventSource)
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
    [causeId, targetId, sourceId, projectileId, flags](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto cause = causeId != 0
        ? RE::TESForm::LookupByID<RE::TESObjectREFR>(causeId)
        : nullptr;
      auto target = targetId != 0
        ? RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId)
        : nullptr;

      if (!cause || !target) {
        return;
      }

      auto sourceForm =
        sourceId != 0 ? RE::TESForm::LookupByID(sourceId) : nullptr;
      auto projectileForm =
        projectileId != 0 ? RE::TESForm::LookupByID(projectileId) : nullptr;

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

  auto objectInitializedId = event->objectInitialized.get()
    ? event->objectInitialized.get()->GetFormID()
    : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [objectInitializedId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto objectInitialized =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(objectInitializedId);

      if (!objectInitialized && objectInitializedId != 0) {
        return;
      }

      AddObjProperty(&obj, "initializedObject", objectInitialized,
                     "ObjectReference");

      SendEvent("scriptInit", obj);
    });

  return EventResult::kContinue;
}

// TODO: Look into LoadGame event
EventResult EventHandler::ProcessEvent(
  const RE::TESLoadGameEvent*, RE::BSTEventSource<RE::TESLoadGameEvent>*)
{
  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [](Napi::Env) { SendEvent("loadGame"); });

  return EventResult::kContinue;
}
EventResult EventHandler::ProcessEvent(
  const RE::TESLockChangedEvent* event,
  RE::BSTEventSource<RE::TESLockChangedEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto lockedObjectId =
    event->lockedObject.get() ? event->lockedObject.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [lockedObjectId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto lockedObject =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(lockedObjectId);

      if (!lockedObject && lockedObjectId != 0) {
        return;
      }

      AddObjProperty(&obj, "lockedObject", lockedObject, "ObjectReference");

      SendEvent("lockChanged", obj);
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

  uint32_t casterId =
    event->caster.get() ? event->caster.get()->GetFormID() : 0;
  uint32_t targetId =
    event->target.get() ? event->target.get()->GetFormID() : 0;
  uint32_t effectId = event->magicEffect;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [casterId, targetId, effectId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto effect = RE::TESForm::LookupByID(effectId);

      auto casterRefr = RE::TESForm::LookupByID<RE::TESObjectREFR>(casterId);
      auto targetRefr = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);

      if (!casterRefr && casterId != 0) {
        return;
      }

      if (!targetRefr && targetId != 0) {
        return;
      }

      AddObjProperty(&obj, "effect", effect, "MagicEffect");
      AddObjProperty(&obj, "caster", casterRefr, "ObjectReference");
      AddObjProperty(&obj, "target", targetRefr, "ObjectReference");

      SendEvent("magicEffectApply", obj);
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

  auto spellId = event->spell;
  auto casterId = event->caster.get() ? event->caster.get()->GetFormID() : 0;
  auto targetId = event->target.get() ? event->target.get()->GetFormID() : 0;
  auto status = static_cast<int>(event->status);

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [spellId, casterId, targetId, status](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto spell = RE::TESForm::LookupByID(spellId);
      auto caster = RE::TESForm::LookupByID<RE::Actor>(casterId);
      auto target = RE::TESForm::LookupByID<RE::Actor>(targetId);

      if (!caster && casterId != 0) {
        return;
      }

      if (!target && targetId != 0) {
        return;
      }

      AddObjProperty(&obj, "caster", caster, "ObjectReference");
      AddObjProperty(&obj, "target", target, "ObjectReference");
      AddObjProperty(&obj, "spell", spell, "Spell");
      AddObjProperty(&obj, "status", status);

      SendEvent("wardHit", obj);
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

  auto movedRefId =
    event->movedRef.get() ? event->movedRef.get()->GetFormID() : 0;
  bool isCellAttached = event->isCellAttached;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [movedRefId, isCellAttached](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto movedRef = RE::TESForm::LookupByID<RE::TESObjectREFR>(movedRefId);

      if (!movedRef && movedRefId != 0) {
        return;
      }

      AddObjProperty(&obj, "movedRef", movedRef, "ObjectReference");
      AddObjProperty(&obj, "isCellAttached", isCellAttached);

      SendEvent("moveAttachDetach", obj);
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

  uint32_t formId = event->formID;
  bool loaded = event->loaded;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [formId, loaded](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto object = RE::TESForm::LookupByID(formId);

      if (!object && formId != 0) {
        return;
      }

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

  auto refId = event->refr.get() ? event->refr.get()->GetFormID() : 0;
  auto eventType = event->type;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [refId, eventType](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto reference = RE::TESForm::LookupByID<RE::TESObjectREFR>(refId);

      if (!reference && refId != 0) {
        return;
      }

      AddObjProperty(&obj, "reference", reference, "ObjectReference");

      switch (eventType) {
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

  auto activeRefId =
    event->activeRef.get() ? event->activeRef.get()->GetFormID() : 0;
  auto refId = event->ref.get() ? event->ref.get()->GetFormID() : 0;
  bool opened = event->opened;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [activeRefId, refId, opened](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto activeRef = RE::TESForm::LookupByID<RE::TESObjectREFR>(activeRefId);
      if (!activeRef && activeRefId != 0) {
        return;
      }

      auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(refId);
      if (!ref && refId != 0) {
        return;
      }

      AddObjProperty(&obj, "cause", activeRef, "ObjectReference");
      AddObjProperty(&obj, "target", ref, "ObjectReference");

      if (opened) {
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

  uint32_t actorId = event->actor.get() ? event->actor.get()->GetFormID() : 0;
  uint32_t packageId = event->package;
  auto type = event->type;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [actorId, packageId, type](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);
      if (!actor && actorId != 0) {
        return;
      }

      auto package = RE::TESForm::LookupByID(packageId);
      if (!package && packageId != 0) {
        return;
      }

      AddObjProperty(&obj, "actor", actor, "ObjectReference");
      AddObjProperty(&obj, "package", package, "Package");

      switch (type) {
        case RE::TESPackageEvent::EventType::kStart:
          SendEvent("packageStart", obj);
          break;
        case RE::TESPackageEvent::EventType::kChange:
          SendEvent("packageChange", obj);
          break;
        case RE::TESPackageEvent::EventType::kEnd:
          SendEvent("packageEnd", obj);
          break;
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

  auto causeId = event->cause.get() ? event->cause.get()->GetFormID() : 0;
  auto targetId = event->target.get() ? event->target.get()->GetFormID() : 0;
  auto perkId = event->perkId;
  auto flag = event->flag;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [causeId, targetId, perkId, flag](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto cause = RE::TESForm::LookupByID<RE::TESObjectREFR>(causeId);
      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);
      auto perk = RE::TESForm::LookupByID<RE::BGSPerk>(perkId);

      if ((!cause && causeId != 0) || (!target && targetId != 0)) {
        return;
      }

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

  auto weaponId = event->weapon;
  auto ammoId = event->ammo;
  auto shotPower = event->shotPower;
  auto isSunGazing = event->isSunGazing;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [weaponId, ammoId, shotPower, isSunGazing](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto weapon = RE::TESForm::LookupByID<RE::TESObjectWEAP>(weaponId);
      auto ammo = RE::TESForm::LookupByID<RE::TESAmmo>(ammoId);

      if (!weapon && weaponId != 0) {
        return;
      }

      if (!ammo && ammoId != 0) {
        return;
      }

      AddObjProperty(&obj, "weapon", weapon, "Weapon");
      AddObjProperty(&obj, "ammo", ammo, "Ammo");
      AddObjProperty(&obj, "power", shotPower);
      AddObjProperty(&obj, "isSunGazing", isSunGazing);

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

  uint32_t questId = event->questId;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([questId](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto quest = RE::TESForm::LookupByID<RE::TESQuest>(questId);

    if (!quest && questId != 0) {
      return;
    }

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

  auto questId = event->formID;
  auto stage = event->stage;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [questId, stage](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto quest = RE::TESForm::LookupByID<RE::TESQuest>(questId);
      if (!quest && questId != 0) {
        return;
      }

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

  uint32_t formId = event->formID;
  bool started = event->started;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [formId, started](Napi::Env env) {
      auto obj = Napi::Object::New(env);
      auto quest = RE::TESForm::LookupByID(formId);
      if (!quest && formId != 0) {
        return;
      }

      AddObjProperty(&obj, "quest", quest, "Quest");

      if (started) {
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

  auto objectId = event->object.get() ? event->object.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([objectId](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto object = RE::TESForm::LookupByID<RE::TESObjectREFR>(objectId);

    if (!object && objectId != 0) {
      return;
    }

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

  auto sellerId = event->seller.get() ? event->seller.get()->GetFormID() : 0;
  auto targetId = event->target.get() ? event->target.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [sellerId, targetId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto seller = RE::TESForm::LookupByID<RE::Actor>(sellerId);
      auto target = RE::TESForm::LookupByID<RE::Actor>(targetId);

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
  auto actorAliasId = event->actorAliasId;
  auto actionIndex = event->actionIndex;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [sceneId, questId, actorAliasId, actionIndex](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto scene = RE::TESForm::LookupByID<RE::BGSScene>(sceneId);
      if (!scene && sceneId != 0)
        return;

      auto quest = RE::TESForm::LookupByID<RE::TESQuest>(questId);
      if (!quest && questId != 0)
        return;

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

  auto sleepStartTime = event->sleepStartTime;
  auto desiredSleepEndTime = event->desiredSleepEndTime;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [sleepStartTime, desiredSleepEndTime](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      AddObjProperty(&obj, "startTime", sleepStartTime);
      AddObjProperty(&obj, "desiredStopTime", desiredSleepEndTime);

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

  bool interrupted = event->interrupted;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([interrupted](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    AddObjProperty(&obj, "isInterrupted", interrupted);

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

  auto casterId = event->object.get() ? event->object.get()->GetFormID() : 0;
  auto spellId = event->spell;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [casterId, spellId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto caster = RE::TESForm::LookupByID<RE::Actor>(casterId);
      auto spell = RE::TESForm::LookupByID<RE::SpellItem>(spellId);

      if (!caster && casterId != 0) {
        return;
      }

      if (!(spell && spell->IsMagicItem())) {
        spdlog::error(
          "ProcessEvent TESSpellCastEvent error! spell not a MagicItem");
        return;
      }

      if (!caster) {
        spdlog::error(
          "ProcessEvent TESSpellCastEvent error! caster == nullptr)");
        return;
      }

      auto castingSource = RE::MagicSystem::CastingSource::kLeftHand;
      bool isCasterValid = false;

      for (const auto magicCaster : caster->magicCasters) {

        if (!magicCaster) {
          continue;
        }

        if (magicCaster->currentSpell == spell) {
          castingSource = magicCaster->GetCastingSource();
          isCasterValid = true;
          break;
        }
      }

      if (!isCasterValid) {
        return;
      }

      const auto magicCaster = caster->GetMagicCaster(castingSource);

      const auto magicTarget = caster->GetMagicTarget();

      RE::Actor* handleTarget = nullptr;

      if (magicTarget->MagicTargetIsActor()) {
        handleTarget =
          reinterpret_cast<RE::Actor*>(magicTarget->GetTargetStatsObject());
      }

      AddObjProperty(&obj, "caster", caster, "ObjectReference");
      AddObjProperty(&obj, "target", handleTarget, "ObjectReference");
      AddObjProperty(&obj, "spell", spell, "Spell");
      AddObjProperty(&obj, "isDualCasting", magicCaster->GetIsDualCasting());
      AddObjProperty(&obj, "castingSource",
                     static_cast<uint32_t>(castingSource));

      SendEvent("spellCast", obj);
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

  auto subjectId =
    event->subject.get() ? event->subject.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([subjectId](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto subject = RE::TESForm::LookupByID<RE::Actor>(subjectId);

    if (!subject && subjectId != 0) {
      return;
    }

    AddObjProperty(&obj, "subject", subject, "ObjectReference");

    SendEvent("switchRaceComplete", obj);
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

  auto statName = std::string(
    event->stat.c_str()); // Properly convert BSFixedString to std::string
  auto newValue = event->value;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [statName, newValue](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      AddObjProperty(&obj, "statName", statName.c_str());
      AddObjProperty(&obj, "newValue", newValue);

      SendEvent("trackedStats", obj);
    });
  return EventResult::kContinue;
}
EventResult EventHandler::ProcessEvent(
  const RE::TESTriggerEnterEvent* event,
  RE::BSTEventSource<RE::TESTriggerEnterEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  uint32_t causeId =
    event->caster.get() ? event->caster.get()->GetFormID() : 0;
  uint32_t targetId =
    event->target.get() ? event->target.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [causeId, targetId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto cause = RE::TESForm::LookupByID<RE::TESObjectREFR>(causeId);
      if (!cause && causeId != 0) {
        return;
      }

      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);
      if (!target && targetId != 0) {
        return;
      }

      AddObjProperty(&obj, "cause", cause, "ObjectReference");
      AddObjProperty(&obj, "target", target, "ObjectReference");

      SendEvent("triggerEnter", obj);
    });

  return EventResult::kContinue;
}
EventResult EventHandler::ProcessEvent(
  const RE::TESTriggerEvent* event, RE::BSTEventSource<RE::TESTriggerEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto casterId = event->caster.get() ? event->caster.get()->GetFormID() : 0;
  auto targetId = event->target.get() ? event->target.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [casterId, targetId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto caster = RE::TESForm::LookupByID<RE::TESObjectREFR>(casterId);
      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);

      if (!caster && casterId != 0) {
        return;
      }
      if (!target && targetId != 0) {
        return;
      }

      AddObjProperty(&obj, "cause", caster, "ObjectReference");
      AddObjProperty(&obj, "target", target, "ObjectReference");

      SendEvent("trigger", obj);
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

  uint32_t casterId =
    event->caster.get() ? event->caster.get()->GetFormID() : 0;
  uint32_t targetId =
    event->target.get() ? event->target.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [casterId, targetId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto caster = RE::TESForm::LookupByID<RE::TESObjectREFR>(casterId);
      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetId);

      if (!caster && casterId != 0) {
        return;
      }
      if (!target && targetId != 0) {
        return;
      }

      AddObjProperty(&obj, "cause", caster, "ObjectReference");
      AddObjProperty(&obj, "target", target, "ObjectReference");

      SendEvent("triggerLeave", obj);
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

  auto oldBaseID = event->oldBaseID;
  auto newBaseID = event->newBaseID;
  auto oldUniqueID = event->oldUniqueID;
  auto newUniqueID = event->newUniqueID;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [oldBaseID, newBaseID, oldUniqueID, newUniqueID](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      AddObjProperty(&obj, "oldBaseID", oldBaseID);
      AddObjProperty(&obj, "newBaseID", newBaseID);
      AddObjProperty(&obj, "oldUniqueID", oldUniqueID);
      AddObjProperty(&obj, "newUniqueID", newUniqueID);

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

  auto waitStartTime = event->waitStartTime;
  auto desiredWaitEndTime = event->desiredWaitEndTime;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [waitStartTime, desiredWaitEndTime](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      AddObjProperty(&obj, "startTime", waitStartTime);
      AddObjProperty(&obj, "desiredStopTime", desiredWaitEndTime);

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

  auto interrupted = event->interrupted;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([interrupted](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    AddObjProperty(&obj, "isInterrupted", interrupted);

    SendEvent("waitStop", obj);
  });

  return EventResult::kContinue;
}
EventResult EventHandler::ProcessEvent(
  const SKSE::ActionEvent* event,
  RE::BSTEventSource<SKSE::ActionEvent>* source)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto actorId = event->actor ? event->actor->GetFormID() : 0;
  auto sourceFormId = event->sourceForm ? event->sourceForm->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [actorId, sourceFormId, event](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto actor = RE::TESForm::LookupByID<RE::Actor>(actorId);
      if (!actor && actorId != 0) {
        return;
      }

      auto sourceForm = RE::TESForm::LookupByID(sourceFormId);
      if (!sourceForm && sourceFormId != 0) {
        return;
      }

      AddObjProperty(&obj, "actor", actor, "Actor");
      AddObjProperty(&obj, "source", sourceForm, "Form");
      AddObjProperty(&obj, "slot", static_cast<int>(event->slot.get()));
      AddObjProperty(&obj, "type", static_cast<int>(event->type.get()));

      // Define the output event name based on the type of action event
      std::string eventName;
      switch (event->type.get()) {
        case SKSE::ActionEvent::Type::kWeaponSwing:
          eventName = "actionWeaponSwing";
          break;
        case SKSE::ActionEvent::Type::kBeginDraw:
          eventName = "actionBeginDraw";
          break;
        case SKSE::ActionEvent::Type::kEndDraw:
          eventName = "actionEndDraw";
          break;
        case SKSE::ActionEvent::Type::kBowDraw:
          eventName = "actionBowDraw";
          break;
        case SKSE::ActionEvent::Type::kBowRelease:
          eventName = "actionBowRelease";
          break;
        case SKSE::ActionEvent::Type::kBeginSheathe:
          eventName = "actionBeginSheathe";
          break;
        case SKSE::ActionEvent::Type::kEndSheathe:
          eventName = "actionEndSheathe";
          break;
        case SKSE::ActionEvent::Type::kSpellCast:
          eventName = "actionSpellCast";
          break;
        case SKSE::ActionEvent::Type::kSpellFire:
          eventName = "actionSpellFire";
          break;
        case SKSE::ActionEvent::Type::kVoiceCast:
          eventName = "actionVoiceCast";
          break;
        case SKSE::ActionEvent::Type::kVoiceFire:
          eventName = "actionVoiceFire";
          break;
      }

      SendEvent(eventName.c_str(), obj);
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

  uint32_t oldStateId = (event && event->oldState)
    ? to_underlying(event->oldState->id)
    : uint32_t(-1);
  uint32_t newStateId = (event && event->newState)
    ? to_underlying(event->newState->id)
    : uint32_t(-1);

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [oldStateId, newStateId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      if (oldStateId == uint32_t(-1)) {
        obj.Set("oldStateId", env.Null());
      } else {
        AddObjProperty(&obj, "oldStateId", oldStateId);
      }

      if (newStateId == uint32_t(-1)) {
        obj.Set("newStateId", env.Null());
      } else {
        AddObjProperty(&obj, "newStateId", newStateId);
      }

      SendEvent("cameraStateChanged", obj);
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

  uint32_t crosshairRefId =
    event->crosshairRef.get() ? event->crosshairRef.get()->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [crosshairRefId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto crosshairRef =
        RE::TESForm::LookupByID<RE::TESObjectREFR>(crosshairRefId);

      if (!crosshairRef && crosshairRefId != 0) {
        return;
      }

      AddObjProperty(&obj, "reference", crosshairRef, "ObjectReference");

      SendEvent("crosshairRefChanged", obj);
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

  uint32_t referenceId = event->reference ? event->reference->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([referenceId](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto reference = RE::TESForm::LookupByID<RE::TESObjectREFR>(referenceId);

    if (!reference && referenceId != 0) {
      return;
    }

    AddObjProperty(&obj, "reference", reference, "ObjectReference");

    SendEvent("niNodeUpdate", obj);
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
  auto eventName = event->eventName;
  auto strArg = event->strArg;
  auto numArg = event->numArg;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [senderId, eventName, strArg, numArg](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto sender = RE::TESForm::LookupByID(senderId);
      if (!sender && senderId != 0) {
        return;
      }

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
  bool opening = event->opening;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [menuName, opening](Napi::Env env) {
      auto obj = Napi::Object::New(env);

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

  struct EventDataBase
  {
    virtual ~EventDataBase() = default;
    virtual const char* GetSPEventName() = 0;
    virtual Napi::Object ToJavaScriptObject(Napi::Env env) = 0;

    void Send(Napi::Env env)
    {
      auto obj = ToJavaScriptObject(env);
      SendEvent(GetSPEventName(), obj);
    }
  };

  struct ButtonEventData : public EventDataBase
  {
    explicit ButtonEventData(const RE::ButtonEvent& buttonEvent)
    {
      device = to_underlying(buttonEvent.device.get());
      idCode = buttonEvent.idCode;
      userEvent = buttonEvent.userEvent.c_str();
      value = buttonEvent.value;
      heldDownSecs = buttonEvent.heldDownSecs;
      isPressed = buttonEvent.IsPressed();
      isUp = buttonEvent.IsUp();
      isDown = buttonEvent.IsDown();
      isHeld = buttonEvent.IsHeld();
      isRepeating = buttonEvent.IsRepeating();
    }

    const char* GetSPEventName() override { return "buttonEvent"; }

    Napi::Object ToJavaScriptObject(Napi::Env env) override
    {
      auto obj = Napi::Object::New(env);
      AddObjProperty(&obj, "device", device);
      AddObjProperty(&obj, "code", idCode);
      AddObjProperty(&obj, "userEventName", userEvent.data());
      AddObjProperty(&obj, "value", value);
      AddObjProperty(&obj, "heldDuration", heldDownSecs);
      AddObjProperty(&obj, "isPressed", isPressed);
      AddObjProperty(&obj, "isUp", isUp);
      AddObjProperty(&obj, "isDown", isDown);
      AddObjProperty(&obj, "isHeld", isHeld);
      AddObjProperty(&obj, "isRepeating", isRepeating);
      return obj;
    }

    int32_t device = RE::INPUT_DEVICE::kNone;
    uint32_t idCode = 0;
    std::string userEvent;
    float value = 0.f;
    float heldDownSecs = 0.f;
    bool isPressed = false;
    bool isUp = false;
    bool isDown = false;
    bool isHeld = false;
    bool isRepeating = false;
  };

  struct MouseMoveEventData : public EventDataBase
  {
    explicit MouseMoveEventData(const RE::MouseMoveEvent& mouseMoveEvent)
    {
      device = to_underlying(mouseMoveEvent.device.get());
      idCode = mouseMoveEvent.idCode;
      userEvent = mouseMoveEvent.userEvent.c_str();
      inputX = mouseMoveEvent.mouseInputX;
      inputY = mouseMoveEvent.mouseInputY;
    }

    const char* GetSPEventName() override { return "mouseMove"; }

    Napi::Object ToJavaScriptObject(Napi::Env env) override
    {
      auto obj = Napi::Object::New(env);
      AddObjProperty(&obj, "device", device);
      AddObjProperty(&obj, "code", idCode);
      AddObjProperty(&obj, "userEventName", userEvent.data());
      AddObjProperty(&obj, "inputX", inputX);
      AddObjProperty(&obj, "inputY", inputY);
      return obj;
    }

    int32_t device = RE::INPUT_DEVICE::kNone;
    uint32_t idCode = 0;
    std::string userEvent;
    float inputX = 0.f;
    float inputY = 0.f;
  };

  struct DeviceConnectEventData : public EventDataBase
  {
    explicit DeviceConnectEventData(
      const RE::DeviceConnectEvent& deviceConnectEvent)
    {
      device = to_underlying(deviceConnectEvent.device.get());
      isConnected = deviceConnectEvent.connected;
    }

    const char* GetSPEventName() override { return "deviceConnect"; }

    Napi::Object ToJavaScriptObject(Napi::Env env) override
    {
      auto obj = Napi::Object::New(env);
      AddObjProperty(&obj, "device", device);
      AddObjProperty(&obj, "isConnected", isConnected);
      return obj;
    }

    int32_t device = RE::INPUT_DEVICE::kNone;
    bool isConnected = false;
  };

  struct ThumbstickEventData : public EventDataBase
  {
    explicit ThumbstickEventData(const RE::ThumbstickEvent& thumbstickEvent)
    {
      device = to_underlying(thumbstickEvent.device.get());
      idCode = thumbstickEvent.idCode;
      userEvent = thumbstickEvent.userEvent.c_str();
      xValue = thumbstickEvent.xValue;
      yValue = thumbstickEvent.yValue;
      isLeft = thumbstickEvent.IsLeft();
      isRight = thumbstickEvent.IsRight();
    }

    const char* GetSPEventName() override { return "thumbstickEvent"; }

    Napi::Object ToJavaScriptObject(Napi::Env env) override
    {
      auto obj = Napi::Object::New(env);
      AddObjProperty(&obj, "device", device);
      AddObjProperty(&obj, "code", idCode);
      AddObjProperty(&obj, "userEventName", userEvent.data());
      AddObjProperty(&obj, "inputX", xValue);
      AddObjProperty(&obj, "inputY", yValue);
      AddObjProperty(&obj, "isLeft", isLeft);
      AddObjProperty(&obj, "isRight", isRight);
      return obj;
    }

    int32_t device = RE::INPUT_DEVICE::kNone;
    uint32_t idCode = 0;
    std::string userEvent;
    float xValue = 0.f;
    float yValue = 0.f;
    bool isLeft = false;
    bool isRight = false;
  };

  struct KinectEventData : public EventDataBase
  {
    explicit KinectEventData(const RE::KinectEvent& kinectEvent)
    {
      device = to_underlying(kinectEvent.device.get());
      idCode = kinectEvent.idCode;
      userEvent = kinectEvent.userEvent.c_str();
      heard = kinectEvent.heard.c_str();
    }

    const char* GetSPEventName() override { return "kinectEvent"; }

    Napi::Object ToJavaScriptObject(Napi::Env env) override
    {
      auto obj = Napi::Object::New(env);
      AddObjProperty(&obj, "device", device);
      AddObjProperty(&obj, "code", idCode);
      AddObjProperty(&obj, "userEventName", userEvent.data());
      AddObjProperty(&obj, "heard", heard.data());
      return obj;
    }

    int32_t device = RE::INPUT_DEVICE::kNone;
    uint32_t idCode = 0;
    std::string userEvent;
    std::string heard;
  };

  for (auto eventItem = *event; eventItem; eventItem = eventItem->next) {
    if (!eventItem) {
      break;
    }

    switch (eventItem->eventType.get()) {
      case RE::INPUT_EVENT_TYPE::kButton: {
        auto buttonEvent = static_cast<RE::ButtonEvent*>(eventItem);
        auto eventData = std::make_shared<ButtonEventData>(*buttonEvent);
        SkyrimPlatform::GetSingleton()->AddUpdateTask(
          [eventData](Napi::Env env) { eventData->Send(env); });
        break;
      }
      case RE::INPUT_EVENT_TYPE::kMouseMove: {
        auto mouseMoveEvent = static_cast<RE::MouseMoveEvent*>(eventItem);
        auto eventData = std::make_shared<MouseMoveEventData>(*mouseMoveEvent);
        SkyrimPlatform::GetSingleton()->AddUpdateTask(
          [eventData](Napi::Env env) { eventData->Send(env); });
        break;
      }
      case RE::INPUT_EVENT_TYPE::kDeviceConnect: {
        auto deviceConnectEvent =
          static_cast<RE::DeviceConnectEvent*>(eventItem);
        auto eventData =
          std::make_shared<DeviceConnectEventData>(*deviceConnectEvent);
        SkyrimPlatform::GetSingleton()->AddUpdateTask(
          [eventData](Napi::Env env) { eventData->Send(env); });
        break;
      }
      case RE::INPUT_EVENT_TYPE::kThumbstick: {
        auto thumbstickEvent = static_cast<RE::ThumbstickEvent*>(eventItem);
        auto eventData =
          std::make_shared<ThumbstickEventData>(*thumbstickEvent);
        SkyrimPlatform::GetSingleton()->AddUpdateTask(
          [eventData](Napi::Env env) { eventData->Send(env); });
        break;
      }
      case RE::INPUT_EVENT_TYPE::kKinect: {
        auto kinectEvent = static_cast<RE::KinectEvent*>(eventItem);
        auto eventData = std::make_shared<KinectEventData>(*kinectEvent);
        SkyrimPlatform::GetSingleton()->AddUpdateTask(
          [eventData](Napi::Env env) { eventData->Send(env); });
        break;
      }
    }
  }

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::BGSFootstepEvent* event,
  RE::BSTEventSource<RE::BGSFootstepEvent>* source)
{
  if (!event) {
    return EventResult::kContinue;
  }

  // Copy only necessary data from the event
  auto tagCopy = event->tag;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([tagCopy](Napi::Env env) {
    auto obj = Napi::Object::New(env);
    AddObjProperty(&obj, "tag", tagCopy);
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

  auto eventType = static_cast<int>(event->type.underlying());

  SkyrimPlatform::GetSingleton()->AddUpdateTask([eventType](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    AddObjProperty(&obj, "eventType", eventType);

    SendEvent("positionPlayer", obj);
  });

  return EventResult::kContinue;
}
EventResult EventHandler::ProcessEvent(
  const RE::ActorKill::Event* event,
  RE::BSTEventSource<RE::ActorKill::Event>* source)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto killerId = event->killer ? event->killer->GetFormID() : 0;
  auto victimId = event->victim ? event->victim->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [killerId, victimId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto killer = RE::TESForm::LookupByID<RE::Actor>(killerId);
      auto victim = RE::TESForm::LookupByID<RE::Actor>(victimId);

      if (!killer && killerId != 0) {
        return;
      }
      if (!victim && victimId != 0) {
        return;
      }

      AddObjProperty(&obj, "killer", killer, "Actor");
      AddObjProperty(&obj, "victim", victim, "Actor");

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

  uint32_t bookId = event->book ? event->book->formID : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([bookId](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto book = RE::TESForm::LookupByID(bookId);

    if (!book && bookId != 0) {
      return;
    }

    AddObjProperty(&obj, "book", book, "Book");

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
  bool isSneakHit = event->sneakHit;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [aggressorId, weaponId, isSneakHit](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto aggressor = RE::TESForm::LookupByID<RE::Actor>(aggressorId);
      auto weapon = RE::TESForm::LookupByID<RE::TESObjectWEAP>(weaponId);

      if (!aggressor && aggressorId != 0) {
        return;
      }

      if (!weapon && weaponId != 0) {
        return;
      }

      AddObjProperty(&obj, "aggressor", aggressor, "ObjectReference");
      AddObjProperty(&obj, "weapon", weapon, "Weapon");
      AddObjProperty(&obj, "isSneakHit", isSneakHit);

      SendEvent("criticalHit", obj);
    });

  return EventResult::kContinue;
}
EventResult EventHandler::ProcessEvent(
  const RE::DisarmedEvent::Event* event,
  RE::BSTEventSource<RE::DisarmedEvent::Event>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto sourceId = event->source ? event->source->GetFormID() : 0;
  auto targetId = event->target ? event->target->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [sourceId, targetId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto source = RE::TESForm::LookupByID<RE::Actor>(sourceId);
      auto target = RE::TESForm::LookupByID<RE::Actor>(targetId);

      if (!source || !target) {
        return;
      }

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

  auto souls = event->souls;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([souls](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    AddObjProperty(&obj, "souls", souls);

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

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [produceItemId, harvesterId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto produceItem = RE::TESForm::LookupByID(produceItemId);
      auto harvester = RE::TESForm::LookupByID<RE::Actor>(harvesterId);

      if (!produceItem && produceItemId != 0) {
        return;
      }

      if (!harvester && harvesterId != 0) {
        return;
      }

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

  auto playerId = event->player ? event->player->GetFormID() : 0;
  auto newLevel = event->newLevel;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [playerId, newLevel](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto player = RE::TESForm::LookupByID<RE::Actor>(playerId);

      if (!player && playerId != 0) {
        return;
      }

      AddObjProperty(&obj, "player", player, "Actor");
      AddObjProperty(&obj, "newLevel", newLevel);

      SendEvent("levelIncrease", obj);
    });

  return EventResult::kContinue;
}
EventResult EventHandler::ProcessEvent(
  const RE::LocationDiscovery::Event* event,
  RE::BSTEventSource<RE::LocationDiscovery::Event>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto worldspaceID = event->worldspaceID;
  auto name = event->mapMarkerData->locationName.fullName;
  auto type = static_cast<uint16_t>(event->mapMarkerData->type.get());
  bool isVisible =
    event->mapMarkerData->flags.any(RE::MapMarkerData::Flag::kVisible);
  bool canTravelTo =
    event->mapMarkerData->flags.any(RE::MapMarkerData::Flag::kCanTravelTo);
  bool isShowAllHidden =
    event->mapMarkerData->flags.any(RE::MapMarkerData::Flag::kShowAllHidden);

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [worldspaceID, name, type, isVisible, canTravelTo,
     isShowAllHidden](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      AddObjProperty(&obj, "worldSpaceId", worldspaceID);
      AddObjProperty(&obj, "name", name);
      AddObjProperty(&obj, "markerType", type);
      AddObjProperty(&obj, "isVisible", isVisible);
      AddObjProperty(&obj, "canTravelTo", canTravelTo);
      AddObjProperty(&obj, "isShowAllHidden", isShowAllHidden);

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

  auto shoutId = event->shout ? event->shout->formID : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([shoutId](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto shout = RE::TESForm::LookupByID<RE::TESShout>(shoutId);
    if (!shout && shoutId != 0) {
      return;
    }

    AddObjProperty(&obj, "shout", shout, "Shout");

    SendEvent("shoutAttack", obj);
  });

  return EventResult::kContinue;
}
EventResult EventHandler::ProcessEvent(
  const RE::SkillIncrease::Event* event,
  RE::BSTEventSource<RE::SkillIncrease::Event>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto playerId = event->player->GetFormID();
  auto actorValue = to_underlying(event->actorValue);

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [playerId, actorValue](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto player = RE::TESForm::LookupByID<RE::Actor>(playerId);
      if (!player) {
        return;
      }

      AddObjProperty(&obj, "player", player, "Actor");
      AddObjProperty(&obj, "actorValue", actorValue);

      SendEvent("skillIncrease", obj);
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

  auto trapperId = event->trapper ? event->trapper->formID : 0;
  auto targetId = event->target ? event->target->formID : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [trapperId, targetId](Napi::Env env) {
      auto obj = Napi::Object::New(env);

      auto trapper = RE::TESForm::LookupByID<RE::Actor>(trapperId);
      auto target = RE::TESForm::LookupByID<RE::Actor>(targetId);

      if ((!trapper && trapperId != 0) || (!target && targetId != 0)) {
        return;
      }

      AddObjProperty(&obj, "trapper", trapper, "Actor");
      AddObjProperty(&obj, "target", target, "Actor");

      SendEvent("soulsTrapped", obj);
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

  auto spellId = event->spell ? event->spell->GetFormID() : 0;

  SkyrimPlatform::GetSingleton()->AddUpdateTask([spellId](Napi::Env env) {
    auto obj = Napi::Object::New(env);

    auto spell = RE::TESForm::LookupByID<RE::SpellItem>(spellId);

    if (!spell && spellId != 0) {
      return;
    }

    AddObjProperty(&obj, "spell", spell, "Spell");

    SendEvent("spellsLearned", obj);
  });

  return EventResult::kContinue;
}
