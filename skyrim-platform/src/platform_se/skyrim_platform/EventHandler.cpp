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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "actor", e->actor.get(), "Actor");
    AddObjProperty(&obj, "oldLoc", e->oldLoc, "Location");
    AddObjProperty(&obj, "newLoc", e->newLoc, "Location");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "cell", e->cell, "Cell");

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

  auto targetid = target->GetFormID();
  auto actorid = actor->GetFormID();

  auto isCombat = event->newState.any(RE::ACTOR_COMBAT_STATE::kCombat);
  auto isSearching = event->newState.any(RE::ACTOR_COMBAT_STATE::kSearching);

  SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [targetid, actorid, isCombat, isSearching] {
      auto obj = JsValue::Object();

      auto target = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetid);
      if (!target && targetid != 0) {
        return EventResult::kContinue;
      }

      auto actor = RE::TESForm::LookupByID<RE::TESObjectREFR>(actorid);
      if (!actor && actorid != 0) {
        return EventResult::kContinue;
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto contFormOld = RE::TESForm::LookupByID(e->oldContainer);
    auto contFormNew = RE::TESForm::LookupByID(e->newContainer);
    auto baseObjForm = RE::TESForm::LookupByID(e->baseObj);

    AddObjProperty(&obj, "oldContainer", contFormOld, "ObjectReference");
    AddObjProperty(&obj, "newContainer", contFormNew, "ObjectReference");
    AddObjProperty(&obj, "baseObj", baseObjForm, "Form");
    AddObjProperty(&obj, "numItems", e->itemCount);
    AddObjProperty(&obj, "uniqueID", e->uniqueID);
    AddObjProperty(&obj, "reference", e->reference.get().get(),
                   "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "actorDying", e->actorDying.get(), "ObjectReference");
    AddObjProperty(&obj, "actorKiller", e->actorKiller.get(),
                   "ObjectReference");

    e->dead ? SendEvent("deathEnd", obj) : SendEvent("deathStart", obj);
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "target", e->target.get(), "ObjectReference");
    AddObjProperty(&obj, "oldStage", e->oldStage);
    AddObjProperty(&obj, "newStage", e->newStage);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "actor", e->actor.get(), "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto baseObjForm = RE::TESForm::LookupByID(e->baseObject);
    auto originalRefrForm = RE::TESForm::LookupByID(e->originalRefr);

    AddObjProperty(&obj, "actor", e->actor.get(), "ObjectReference");
    AddObjProperty(&obj, "baseObj", baseObjForm, "Form");
    AddObjProperty(&obj, "originalRefr", originalRefrForm, "ObjectReference");
    AddObjProperty(&obj, "uniqueId", e->uniqueID);

    e->equipped ? SendEvent("equip", obj) : SendEvent("unequip", obj);
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "travelTimeGameHours", e->travelTimeGameHours);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "actor", e->actor.get(), "ObjectReference");
    AddObjProperty(&obj, "target", e->targetFurniture.get(),
                   "ObjectReference");

    if (e->type == RE::TESFurnitureEvent::FurnitureEventType::kExit) {
      SendEvent("furnitureExit", obj);
    } else if (e->type == RE::TESFurnitureEvent::FurnitureEventType::kEnter) {
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "refr", e->ref.get(), "ObjectReference");
    AddObjProperty(&obj, "isGrabbed", e->grabbed);

    SendEvent("grabRelease", obj);
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "initializedObject", e->objectInitialized.get(),
                   "ObjectReference");

    SendEvent("scriptInit", obj);
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "lockedObject", e->lockedObject.get(),
                   "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto effect = RE::TESForm::LookupByID(e->magicEffect);

    AddObjProperty(&obj, "effect", effect, "MagicEffect");
    AddObjProperty(&obj, "caster", e->caster.get(), "ObjectReference");
    AddObjProperty(&obj, "target", e->target.get(), "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto spell = RE::TESForm::LookupByID(e->spell);
    auto status = to_underlying(e->status);

    AddObjProperty(&obj, "caster", e->caster.get(), "ObjectReference");
    AddObjProperty(&obj, "target", e->target.get(), "ObjectReference");
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "movedRef", e->movedRef.get(), "ObjectReference");
    AddObjProperty(&obj, "isCellAttached", e->isCellAttached);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto object = RE::TESForm::LookupByID(e->formID);

    AddObjProperty(&obj, "object", object, "Form");
    AddObjProperty(&obj, "isLoaded", e->loaded);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "reference", e->refr.get(), "ObjectReference");

    switch (e->type) {
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "cause", e->activeRef.get(), "ObjectReference");
    AddObjProperty(&obj, "target", e->ref.get(), "ObjectReference");

    if (e->opened) {
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto package = RE::TESForm::LookupByID(e->package);

    AddObjProperty(&obj, "actor", e->actor.get(), "ObjectReference");
    AddObjProperty(&obj, "package", package, "Package");

    switch (e->type) {
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto perk = RE::TESForm::LookupByID(e->perkId);

    AddObjProperty(&obj, "cause", e->cause.get(), "ObjectReference");
    AddObjProperty(&obj, "target", e->target.get(), "ObjectReference");
    AddObjProperty(&obj, "perk", perk, "Perk");
    AddObjProperty(&obj, "flag", e->flag);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto weapon = RE::TESForm::LookupByID(e->weaponId);
    auto ammo = RE::TESForm::LookupByID(e->ammoId);

    AddObjProperty(&obj, "weapon", weapon, "Weapon");
    AddObjProperty(&obj, "ammo", ammo, "Ammo");
    AddObjProperty(&obj, "power", e->power);
    AddObjProperty(&obj, "target", e->isSunGazing);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto quest = RE::TESForm::LookupByID(e->questId);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto quest = RE::TESForm::LookupByID(e->questId);

    AddObjProperty(&obj, "quest", quest, "Quest");
    AddObjProperty(&obj, "stage", e->stage);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto quest = RE::TESForm::LookupByID(e->questId);

    AddObjProperty(&obj, "quest", quest, "Quest");

    if (e->isStarted) {
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "object", e->object.get(), "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "seller", e->seller.get(), "ObjectReference");
    AddObjProperty(&obj, "target", e->target.get(), "ObjectReference");

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
  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto scene = RE::TESForm::LookupByID<RE::BGSScene>(e->sceneId);
    auto quest = RE::TESForm::LookupByID<RE::TESQuest>(e->questId);

    AddObjProperty(&obj, "actorAliasId", e->actorAliasId);
    AddObjProperty(&obj, "actionIndex", e->actionIndex);
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "startTime", e->sleepStartTime);
    AddObjProperty(&obj, "desiredStopTime", e->desiredSleepEndTime);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "isInterrupted", e->isInterrupted);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto spell = RE::TESForm::LookupByID(e->spell);

    AddObjProperty(&obj, "caster", e->caster.get(), "ObjectReference");
    AddObjProperty(&obj, "spell", spell, "Spell");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "subject", e->subject.get(), "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "statName", e->stat.data());
    AddObjProperty(&obj, "newValue", e->value);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "cause", e->caster.get(), "ObjectReference");
    AddObjProperty(&obj, "target", e->target.get(), "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "cause", e->caster.get(), "ObjectReference");
    AddObjProperty(&obj, "target", e->target.get(), "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "cause", e->caster.get(), "ObjectReference");
    AddObjProperty(&obj, "target", e->target.get(), "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "startTime", e->waitStartTime);
    AddObjProperty(&obj, "desiredStopTime", e->desiredWaitEndTime);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "isInterrupted", e->interrupted);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto slot = to_underlying(e->slot.get());

    AddObjProperty(&obj, "actor", e->actor, "Actor");
    AddObjProperty(&obj, "source", e->sourceForm, "Form");
    AddObjProperty(&obj, "slot", slot);

    switch (e->type.get()) {
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto oldStateId = to_underlying(e->oldState->id);
    auto newStateId = to_underlying(e->newState->id);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "reference", e->crosshairRef.get(),
                   "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "reference", e->reference, "ObjectReference");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "sender", e->sender, "Form");
    AddObjProperty(&obj, "eventName", e->eventName);
    AddObjProperty(&obj, "strArg", e->strArg);
    AddObjProperty(&obj, "numArg", e->numArg);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "name", e->menuName);

    if (e->opening) {
      SendEvent("menuOpen", obj);
    } else {
      SendEvent("menuClose", obj);
    }
  });

  return EventResult::kContinue;
};

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "tag", e->tag);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto type = to_underlying(e->type.get());

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "killer", e->killer, "Actor");
    AddObjProperty(&obj, "victim", e->victim, "Actor");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "book", e->book, "Book");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "aggressor", e->aggressor, "ObjectReference");
    AddObjProperty(&obj, "weapon", e->weapon, "Weapon");
    AddObjProperty(&obj, "isSneakHit", e->sneakHit);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "source", e->source, "Actor");
    AddObjProperty(&obj, "target", e->target, "Actor");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "produceItem", e->produceItem, "Form");
    AddObjProperty(&obj, "harvester", e->harvester, "Actor");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "player", e->player, "Actor");
    AddObjProperty(&obj, "newLevel", e->newLevel);

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    auto type = to_underlying(e->mapMarkerData->type.get());

    AddObjProperty(&obj, "worldSpaceId", e->worldspaceID);
    AddObjProperty(&obj, "name", e->mapMarkerData->locationName.fullName);
    AddObjProperty(&obj, "markerType", type);
    AddObjProperty(
      &obj, "isVisible",
      e->mapMarkerData->flags.any(RE::MapMarkerData::Flag::kVisible));
    AddObjProperty(
      &obj, "canTravelTo",
      e->mapMarkerData->flags.any(RE::MapMarkerData::Flag::kCanTravelTo));
    AddObjProperty(
      &obj, "isShowAllHidden",
      e->mapMarkerData->flags.any(RE::MapMarkerData::Flag::kShowAllHidden));

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "shout", e->shout, "Shout");

    SendEvent("shoutAttack", obj);
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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "player", e->player, "Actor");
    AddObjProperty(&obj, "actorValue", to_underlying(e->actorValue));

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "trapper", e->trapper, "Actor");
    AddObjProperty(&obj, "target", e->target, "Actor");

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

  auto e = CopyEventPtr(event);

  SkyrimPlatform::GetSingleton()->AddUpdateTask([e] {
    auto obj = JsValue::Object();

    AddObjProperty(&obj, "spell", e->spell, "Spell");

    SendEvent("spellsLearned", obj);
  });

  return EventResult::kContinue;
}
