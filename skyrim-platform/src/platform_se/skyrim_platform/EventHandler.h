#pragma once
#include "EventUtils.h"

using EventResult = RE::BSEventNotifyControl;

struct Sink
{
  Sink(std::vector<const char*> _events,
       std::function<void(const ::Sink*)> _add,
       std::function<void(const ::Sink*)> _remove,
       std::function<bool(const ::Sink*)> _validate)
    : Activate(_add)
    , Deactivate(_remove)
    , IsActive(_validate)
    , events(_events)
  {
  }

  std::vector<const char*> events;

  std::function<void(const ::Sink*)> Activate;
  std::function<void(const ::Sink*)> Deactivate;
  std::function<bool(const ::Sink*)> IsActive;
};

using SinkSet = robin_hood::unordered_set<const ::Sink*>;

class EventHandler final
  : public RE::BSTEventSink<RE::TESActivateEvent>
  , public RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>
  , public RE::BSTEventSink<RE::TESActorLocationChangeEvent>
  , public RE::BSTEventSink<RE::TESCellAttachDetachEvent>
  , public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>
  , public RE::BSTEventSink<RE::TESCombatEvent>
  , public RE::BSTEventSink<RE::TESContainerChangedEvent>
  , public RE::BSTEventSink<RE::TESDeathEvent>
  , public RE::BSTEventSink<RE::TESDestructionStageChangedEvent>
  , public RE::BSTEventSink<RE::TESEnterBleedoutEvent>
  , public RE::BSTEventSink<RE::TESEquipEvent>
  , public RE::BSTEventSink<RE::TESFastTravelEndEvent>
  , public RE::BSTEventSink<RE::TESFurnitureEvent>
  , public RE::BSTEventSink<RE::TESGrabReleaseEvent>
  , public RE::BSTEventSink<RE::TESHitEvent>
  , public RE::BSTEventSink<RE::TESInitScriptEvent>
  , public RE::BSTEventSink<RE::TESLoadGameEvent>
  , public RE::BSTEventSink<RE::TESLockChangedEvent>
  , public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>
  , public RE::BSTEventSink<RE::TESMagicWardHitEvent>
  , public RE::BSTEventSink<RE::TESMoveAttachDetachEvent>
  , public RE::BSTEventSink<RE::TESObjectLoadedEvent>
  , public RE::BSTEventSink<RE::TESObjectREFRTranslationEvent>
  , public RE::BSTEventSink<RE::TESOpenCloseEvent>
  , public RE::BSTEventSink<RE::TESPackageEvent>
  , public RE::BSTEventSink<RE::TESPerkEntryRunEvent>
  , public RE::BSTEventSink<RE::TESPlayerBowShotEvent>
  , public RE::BSTEventSink<RE::TESQuestInitEvent>
  , public RE::BSTEventSink<RE::TESQuestStageEvent>
  , public RE::BSTEventSink<RE::TESQuestStartStopEvent>
  , public RE::BSTEventSink<RE::TESResetEvent>
  , public RE::BSTEventSink<RE::TESSceneActionEvent>
  , public RE::BSTEventSink<RE::TESSellEvent>
  , public RE::BSTEventSink<RE::TESSleepStartEvent>
  , public RE::BSTEventSink<RE::TESSleepStopEvent>
  , public RE::BSTEventSink<RE::TESSpellCastEvent>
  , public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>
  , public RE::BSTEventSink<RE::TESTrackedStatsEvent>
  , public RE::BSTEventSink<RE::TESTriggerEnterEvent>
  , public RE::BSTEventSink<RE::TESTriggerEvent>
  , public RE::BSTEventSink<RE::TESTriggerLeaveEvent>
  , public RE::BSTEventSink<RE::TESUniqueIDChangeEvent>
  , public RE::BSTEventSink<RE::TESWaitStartEvent>
  , public RE::BSTEventSink<RE::TESWaitStopEvent>
  , public RE::BSTEventSink<SKSE::ActionEvent>
  , public RE::BSTEventSink<SKSE::CameraEvent>
  , public RE::BSTEventSink<SKSE::CrosshairRefEvent>
  , public RE::BSTEventSink<SKSE::NiNodeUpdateEvent>
  , public RE::BSTEventSink<SKSE::ModCallbackEvent>
  , public RE::BSTEventSink<RE::BGSFootstepEvent>
  , public RE::BSTEventSink<RE::InputEvent*>
  , public RE::BSTEventSink<RE::MenuOpenCloseEvent>
  , public RE::BSTEventSink<RE::PositionPlayerEvent>
  , public RE::BSTEventSink<RE::ActorKill::Event>
  , public RE::BSTEventSink<RE::BooksRead::Event>
  , public RE::BSTEventSink<RE::CriticalHit::Event>
  , public RE::BSTEventSink<RE::DisarmedEvent::Event>
  , public RE::BSTEventSink<RE::DragonSoulsGained::Event>
  , public RE::BSTEventSink<RE::ItemHarvested::Event>
  , public RE::BSTEventSink<RE::LevelIncrease::Event>
  , public RE::BSTEventSink<RE::LocationDiscovery::Event>
  , public RE::BSTEventSink<RE::ShoutAttack::Event>
  , public RE::BSTEventSink<RE::SkillIncrease::Event>
  , public RE::BSTEventSink<RE::SoulsTrapped::Event>
  , public RE::BSTEventSink<RE::SpellsLearned::Event>
{
public:
  [[nodiscard]] static EventHandler* GetSingleton()
  {
    static EventHandler singleton;
    return &singleton;
  }

  static void HandleSKSEMessage(SKSE::MessagingInterface::Message* msg);

  static void SendSimpleEventOnUpdate(const char* eventName);
  static void SendSimpleEventOnTick(const char* eventName);
  static void SendEventOnUpdate(const char* eventName, const JsValue& obj);
  static void SendEventOnTick(const char* eventName, const JsValue& obj);
  static void SendEventConsoleMsg(const char* msg);

  void DeactivateAllSinks()
  {
    if (activeSinks.empty()) {
      return;
    }

    for (const auto& sink : activeSinks) {
      sink->Deactivate(sink);
    }
  }

  SinkSet* GetSinks() { return &sinks; }
  bool IsActiveSink(const Sink* sink) { return activeSinks.contains(sink); }

  /**
   * @brief Registers sink using script event source.
   */
  template <class E>
  void ActivateSink(const Sink* sink)
  {
    if (const auto source = ::GetEventSource<E>()) {
      source->AddEventSink(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(E).name());
      activeSinks.emplace(sink);
    }
  }

  /**
   * @brief Registers sink using specific event source.
   */
  template <class E>
  void ActivateSink(const Sink* sink, RE::BSTEventSource<E>* source)
  {
    if (source) {
      source->AddEventSink(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(E).name());
      activeSinks.emplace(sink);
    }
  }

  /**
   * @brief Registers sink using specific class as event source.
   */
  template <class T, class E>
  void ActivateSink(const Sink* sink)
  {
    if (const auto source = ::GetEventSource<T, E>()) {
      source->AddEventSink(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(E).name());
      activeSinks.emplace(sink);
    }
  }

  /**
   * @brief Unregisters sink using script event source.
   */
  template <class E>
  void DeactivateSink(const Sink* sink)
  {
    if (const auto source = ::GetEventSource<E>()) {
      source->RemoveEventSink(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
      activeSinks.erase(sink);
    }
  }

  /**
   * @brief Unregisters sink using specific event source.
   */
  template <class E>
  void DeactivateSink(const Sink* sink, RE::BSTEventSource<E>* source)
  {
    if (source) {
      source->RemoveEventSink(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
      activeSinks.erase(sink);
    }
  }

  /**
   * @brief Unregisters sink using specific class as event source.
   */
  template <class T, class E>
  void DeactivateSink(const Sink* sink)
  {
    if (const auto source = ::GetEventSource<T, E>()) {
      source->RemoveEventSink(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
      activeSinks.erase(sink);
    }
  }

  /* Process event block start */
  EventResult ProcessEvent(const RE::TESActivateEvent* event,
                           RE::BSTEventSource<RE::TESActivateEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESActiveEffectApplyRemoveEvent* event,
    RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESActorLocationChangeEvent* event,
    RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESCellAttachDetachEvent* event,
    RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESCellFullyLoadedEvent* event,
    RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override;

  EventResult ProcessEvent(const RE::TESCombatEvent* event,
                           RE::BSTEventSource<RE::TESCombatEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESContainerChangedEvent* event,
    RE::BSTEventSource<RE::TESContainerChangedEvent>*) override;

  EventResult ProcessEvent(const RE::TESDeathEvent* event,
                           RE::BSTEventSource<RE::TESDeathEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESDestructionStageChangedEvent* event,
    RE::BSTEventSource<RE::TESDestructionStageChangedEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESEnterBleedoutEvent* event,
    RE::BSTEventSource<RE::TESEnterBleedoutEvent>*) override;

  EventResult ProcessEvent(const RE::TESEquipEvent* event,
                           RE::BSTEventSource<RE::TESEquipEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESFastTravelEndEvent* event,
    RE::BSTEventSource<RE::TESFastTravelEndEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESFurnitureEvent* event,
    RE::BSTEventSource<RE::TESFurnitureEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESGrabReleaseEvent* event,
    RE::BSTEventSource<RE::TESGrabReleaseEvent>*) override;

  EventResult ProcessEvent(const RE::TESHitEvent* event,
                           RE::BSTEventSource<RE::TESHitEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESInitScriptEvent* event,
    RE::BSTEventSource<RE::TESInitScriptEvent>*) override;

  EventResult ProcessEvent(const RE::TESLoadGameEvent* event,
                           RE::BSTEventSource<RE::TESLoadGameEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESLockChangedEvent* event,
    RE::BSTEventSource<RE::TESLockChangedEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESMagicEffectApplyEvent* event,
    RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESMagicWardHitEvent* event,
    RE::BSTEventSource<RE::TESMagicWardHitEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESMoveAttachDetachEvent* event,
    RE::BSTEventSource<RE::TESMoveAttachDetachEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESObjectLoadedEvent* event,
    RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESObjectREFRTranslationEvent* event,
    RE::BSTEventSource<RE::TESObjectREFRTranslationEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESOpenCloseEvent* event,
    RE::BSTEventSource<RE::TESOpenCloseEvent>*) override;

  EventResult ProcessEvent(const RE::TESPackageEvent* event,
                           RE::BSTEventSource<RE::TESPackageEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESPerkEntryRunEvent* event,
    RE::BSTEventSource<RE::TESPerkEntryRunEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESPlayerBowShotEvent* event,
    RE::BSTEventSource<RE::TESPlayerBowShotEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESQuestInitEvent* event,
    RE::BSTEventSource<RE::TESQuestInitEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESQuestStageEvent* event,
    RE::BSTEventSource<RE::TESQuestStageEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESQuestStartStopEvent* event,
    RE::BSTEventSource<RE::TESQuestStartStopEvent>*) override;

  EventResult ProcessEvent(const RE::TESResetEvent* event,
                           RE::BSTEventSource<RE::TESResetEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSceneActionEvent* event,
    RE::BSTEventSource<RE::TESSceneActionEvent>*) override;

  EventResult ProcessEvent(const RE::TESSellEvent* event,
                           RE::BSTEventSource<RE::TESSellEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSleepStartEvent* event,
    RE::BSTEventSource<RE::TESSleepStartEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSleepStopEvent* event,
    RE::BSTEventSource<RE::TESSleepStopEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSpellCastEvent* event,
    RE::BSTEventSource<RE::TESSpellCastEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSwitchRaceCompleteEvent* event,
    RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESTrackedStatsEvent* event,
    RE::BSTEventSource<RE::TESTrackedStatsEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESTriggerEnterEvent* event,
    RE::BSTEventSource<RE::TESTriggerEnterEvent>*) override;

  EventResult ProcessEvent(const RE::TESTriggerEvent* event,
                           RE::BSTEventSource<RE::TESTriggerEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESTriggerLeaveEvent* event,
    RE::BSTEventSource<RE::TESTriggerLeaveEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESUniqueIDChangeEvent* event,
    RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESWaitStartEvent* event,
    RE::BSTEventSource<RE::TESWaitStartEvent>*) override;

  EventResult ProcessEvent(const RE::TESWaitStopEvent* event,
                           RE::BSTEventSource<RE::TESWaitStopEvent>*) override;

  EventResult ProcessEvent(const SKSE::ActionEvent* event,
                           RE::BSTEventSource<SKSE::ActionEvent>*) override;

  EventResult ProcessEvent(const SKSE::CameraEvent* event,
                           RE::BSTEventSource<SKSE::CameraEvent>*) override;

  EventResult ProcessEvent(
    const SKSE::CrosshairRefEvent* event,
    RE::BSTEventSource<SKSE::CrosshairRefEvent>*) override;

  EventResult ProcessEvent(
    const SKSE::NiNodeUpdateEvent* event,
    RE::BSTEventSource<SKSE::NiNodeUpdateEvent>*) override;

  EventResult ProcessEvent(
    const SKSE::ModCallbackEvent* event,
    RE::BSTEventSource<SKSE::ModCallbackEvent>*) override;

  EventResult ProcessEvent(const RE::BGSFootstepEvent* event,
                           RE::BSTEventSource<RE::BGSFootstepEvent>*) override;

  EventResult ProcessEvent(RE::InputEvent* const* event,
                           RE::BSTEventSource<RE::InputEvent*>*) override;

  EventResult ProcessEvent(
    const RE::MenuOpenCloseEvent* event,
    RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

  EventResult ProcessEvent(
    const RE::PositionPlayerEvent* event,
    RE::BSTEventSource<RE::PositionPlayerEvent>*) override;

  EventResult ProcessEvent(const RE::ActorKill::Event* event,
                           RE::BSTEventSource<RE::ActorKill::Event>*) override;

  EventResult ProcessEvent(const RE::BooksRead::Event* event,
                           RE::BSTEventSource<RE::BooksRead::Event>*) override;

  EventResult ProcessEvent(
    const RE::CriticalHit::Event* event,
    RE::BSTEventSource<RE::CriticalHit::Event>*) override;

  EventResult ProcessEvent(
    const RE::DisarmedEvent::Event* event,
    RE::BSTEventSource<RE::DisarmedEvent::Event>*) override;

  EventResult ProcessEvent(
    const RE::DragonSoulsGained::Event* event,
    RE::BSTEventSource<RE::DragonSoulsGained::Event>*) override;

  EventResult ProcessEvent(
    const RE::ItemHarvested::Event* event,
    RE::BSTEventSource<RE::ItemHarvested::Event>*) override;

  EventResult ProcessEvent(
    const RE::LevelIncrease::Event* event,
    RE::BSTEventSource<RE::LevelIncrease::Event>*) override;

  EventResult ProcessEvent(
    const RE::LocationDiscovery::Event* event,
    RE::BSTEventSource<RE::LocationDiscovery::Event>*) override;

  EventResult ProcessEvent(
    const RE::ShoutAttack::Event* event,
    RE::BSTEventSource<RE::ShoutAttack::Event>*) override;

  EventResult ProcessEvent(
    const RE::SkillIncrease::Event* event,
    RE::BSTEventSource<RE::SkillIncrease::Event>*) override;

  EventResult ProcessEvent(
    const RE::SoulsTrapped::Event* event,
    RE::BSTEventSource<RE::SoulsTrapped::Event>*) override;

  EventResult ProcessEvent(
    const RE::SpellsLearned::Event* event,
    RE::BSTEventSource<RE::SpellsLearned::Event>*) override;

  /* ProcessEvent block end */

private:
  EventHandler()
  {
    sinks.reserve(70);
    activeSinks.reserve(20);

    // script events
    AppendSinkScriptEvent<RE::TESActivateEvent>(std::vector({ "activate" }));
    AppendSinkScriptEvent<RE::TESActiveEffectApplyRemoveEvent>(
      std::vector({ "effectStart", "effectFinish" }));
    AppendSinkScriptEvent<RE::TESActorLocationChangeEvent>(
      std::vector({ "locationChanged" }));
    AppendSinkScriptEvent<RE::TESCellAttachDetachEvent>(
      std::vector({ "cellAttach", "cellDetach" }));
    AppendSinkScriptEvent<RE::TESCellFullyLoadedEvent>(
      std::vector({ "cellFullyLoaded" }));
    AppendSinkScriptEvent<RE::TESCombatEvent>(std::vector({ "combatState" }));
    AppendSinkScriptEvent<RE::TESContainerChangedEvent>(
      std::vector({ "containerChanged" }));
    AppendSinkScriptEvent<RE::TESDeathEvent>(
      std::vector({ "deathEnd", "deathStart" }));
    AppendSinkScriptEvent<RE::TESDestructionStageChangedEvent>(
      std::vector({ "destructionStageChanged" }));
    AppendSinkScriptEvent<RE::TESEnterBleedoutEvent>(
      std::vector({ "enterBleedout" }));
    AppendSinkScriptEvent<RE::TESEquipEvent>(
      std::vector({ "equip", "unequip" }));
    AppendSinkScriptEvent<RE::TESFastTravelEndEvent>(
      std::vector({ "fastTravelEnd" }));
    AppendSinkScriptEvent<RE::TESFurnitureEvent>(
      std::vector({ "furnitureExit", "furnitureEnter" }));
    AppendSinkScriptEvent<RE::TESGrabReleaseEvent>(
      std::vector({ "grabRelease" }));
    AppendSinkScriptEvent<RE::TESHitEvent>(std::vector({ "hit" }));
    AppendSinkScriptEvent<RE::TESInitScriptEvent>(
      std::vector({ "scriptInit" }));
    AppendSinkScriptEvent<RE::TESLoadGameEvent>(std::vector({ "loadGame" }));
    AppendSinkScriptEvent<RE::TESLockChangedEvent>(
      std::vector({ "lockChanged" }));
    AppendSinkScriptEvent<RE::TESMagicEffectApplyEvent>(
      std::vector({ "magicEffectApply" }));
    AppendSinkScriptEvent<RE::TESMagicWardHitEvent>(
      std::vector({ "wardHit" }));
    AppendSinkScriptEvent<RE::TESMoveAttachDetachEvent>(
      std::vector({ "moveAttachDetach" }));
    AppendSinkScriptEvent<RE::TESObjectLoadedEvent>(
      std::vector({ "objectLoaded" }));
    AppendSinkScriptEvent<RE::TESObjectREFRTranslationEvent>(
      std::vector({ "translationFailed", "translationAlmostCompleted",
                    "translationCompleted" }));
    AppendSinkScriptEvent<RE::TESOpenCloseEvent>(
      std::vector({ "open", "close" }));
    AppendSinkScriptEvent<RE::TESPackageEvent>(
      std::vector({ "packageStart", "packageChange", "packageEnd" }));
    AppendSinkScriptEvent<RE::TESPerkEntryRunEvent>(
      std::vector({ "perkEntryRun" }));
    AppendSinkScriptEvent<RE::TESPlayerBowShotEvent>(
      std::vector({ "playerBowShot" }));
    AppendSinkScriptEvent<RE::TESQuestInitEvent>(std::vector({ "questInit" }));
    AppendSinkScriptEvent<RE::TESQuestStageEvent>(
      std::vector({ "questStage" }));
    AppendSinkScriptEvent<RE::TESQuestStartStopEvent>(
      std::vector({ "questStart", "questStop" }));
    AppendSinkScriptEvent<RE::TESResetEvent>(std::vector({ "reset" }));
    AppendSinkScriptEvent<RE::TESSceneActionEvent>(
      std::vector({ "sceneAction" }));
    AppendSinkScriptEvent<RE::TESSellEvent>(std::vector({ "sell" }));
    AppendSinkScriptEvent<RE::TESSleepStartEvent>(
      std::vector({ "sleepStart" }));
    AppendSinkScriptEvent<RE::TESSleepStopEvent>(std::vector({ "sleepStop" }));
    AppendSinkScriptEvent<RE::TESSpellCastEvent>(std::vector({ "spellCast" }));
    AppendSinkScriptEvent<RE::TESSwitchRaceCompleteEvent>(
      std::vector({ "switchRaceComplete" }));
    AppendSinkScriptEvent<RE::TESTrackedStatsEvent>(
      std::vector({ "trackedStats" }));
    AppendSinkScriptEvent<RE::TESTriggerEnterEvent>(
      std::vector({ "triggerEnter" }));
    AppendSinkScriptEvent<RE::TESTriggerEvent>(std::vector({ "trigger" }));
    AppendSinkScriptEvent<RE::TESTriggerLeaveEvent>(
      std::vector({ "triggerLeave" }));
    AppendSinkScriptEvent<RE::TESUniqueIDChangeEvent>(
      std::vector({ "uniqueIdChange" }));
    AppendSinkScriptEvent<RE::TESWaitStartEvent>(std::vector({ "waitStart" }));
    AppendSinkScriptEvent<RE::TESWaitStopEvent>(std::vector({ "waitStop" }));

    // skse events
    // at the moment of writing, no way was found to standardize event source
    // aquisition for skse events, which means those must be done manually
    if (const auto source = SKSE::GetActionEventSource()) {
      auto events = std::vector(
        { "actionWeaponSwing", "actionBeginDraw", "actionEndDraw",
          "actionBowDraw", "actionBowRelease", "actionBeginSheathe",
          "actionEndSheathe", "actionSpellCast", "actionSpellFire",
          "actionVoiceCast", "actionVoiceFire" });
      const auto sink = new Sink(
        events,
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetActionEventSource();
          handler->ActivateSink(sink, source);
        },
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetActionEventSource();
          handler->DeactivateSink(sink, source);
        },
        [](const Sink* sink) -> bool {
          const auto handler = EventHandler::GetSingleton();
          return handler->IsActiveSink(sink);
        });

      sinks.emplace(sink);
    }

    if (const auto source = SKSE::GetCameraEventSource()) {
      auto events = std::vector({ "cameraStateChanged" });
      const auto sink = new Sink(
        events,
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetCameraEventSource();
          handler->ActivateSink(sink, source);
        },
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetCameraEventSource();
          handler->DeactivateSink(sink, source);
        },
        [](const Sink* sink) -> bool {
          const auto handler = EventHandler::GetSingleton();
          return handler->IsActiveSink(sink);
        });

      sinks.emplace(sink);
    }

    if (const auto source = SKSE::GetCrosshairRefEventSource()) {
      auto events = std::vector({ "crosshairRefChanged" });
      const auto sink = new Sink(
        events,
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetCrosshairRefEventSource();
          handler->ActivateSink(sink, source);
        },
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetCrosshairRefEventSource();
          handler->DeactivateSink(sink, source);
        },
        [](const Sink* sink) -> bool {
          const auto handler = EventHandler::GetSingleton();
          return handler->IsActiveSink(sink);
        });

      sinks.emplace(sink);
    }

    if (const auto source = SKSE::GetModCallbackEventSource()) {
      auto events = std::vector({ "modEvent" });
      const auto sink = new Sink(
        events,
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetModCallbackEventSource();
          handler->ActivateSink(sink, source);
        },
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetModCallbackEventSource();
          handler->DeactivateSink(sink, source);
        },
        [](const Sink* sink) -> bool {
          auto handler = EventHandler::GetSingleton();
          return handler->IsActiveSink(sink);
        });

      sinks.emplace(sink);
    }

    if (const auto source = SKSE::GetNiNodeUpdateEventSource()) {
      auto events = std::vector({ "niNodeUpdate" });
      const auto sink = new Sink(
        events,
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetNiNodeUpdateEventSource();
          handler->ActivateSink(sink, source);
        },
        [](const Sink* sink) {
          const auto handler = EventHandler::GetSingleton();
          auto source = SKSE::GetNiNodeUpdateEventSource();
          handler->DeactivateSink(sink, source);
        },
        [](const Sink* sink) -> bool {
          const auto handler = EventHandler::GetSingleton();
          return handler->IsActiveSink(sink);
        });

      sinks.emplace(sink);
    }

    // misc events
    AppendSink<RE::UI, RE::MenuOpenCloseEvent>(
      std::vector({ "menuOpen", "menuClose" }));
    AppendSink<RE::BSInputDeviceManager, RE::InputEvent*>(
      std::vector({ "buttonEvent", "mouseMove", "thumbstickEvent",
                    "kinectEvent", "deviceConnect" }));
    AppendSink<RE::BGSFootstepManager, RE::BGSFootstepEvent>(
      std::vector({ "footstep" }));
    AppendSink<RE::PlayerCharacter, RE::PositionPlayerEvent>(
      std::vector({ "positionPlayer" }));

    // story events
    // TODO: implement these
    AppendSink<RE::ActorKill>(std::vector({ "actorKill" }));
    AppendSink<RE::BooksRead>(std::vector({ "bookRead" }));
    AppendSink<RE::CriticalHit>(std::vector({ "criticalHit" }));
    AppendSink<RE::DisarmedEvent>(std::vector({ "disarmedEvent" }));
    AppendSink<RE::DragonSoulsGained>(std::vector({ "dragonSoulsGained" }));
    AppendSink<RE::ItemHarvested>(std::vector({ "itemHarvested" }));
    AppendSink<RE::LevelIncrease>(std::vector({ "levelIncrease" }));
    AppendSink<RE::LocationDiscovery>(std::vector({ "locationDiscovery" }));
    AppendSink<RE::ShoutAttack>(std::vector({ "shoutAttack" }));
    AppendSink<RE::SkillIncrease>(std::vector({ "skillIncrease" }));
    AppendSink<RE::SoulsTrapped>(std::vector({ "soulsTrapped" }));
    AppendSink<RE::SpellsLearned>(std::vector({ "spellsLearned" }));
  }

  template <class E>
  void AppendSinkScriptEvent(std::vector<const char*> eventNames)
  {
    // should consider checking if sink exists
    // but since we store sink pointers
    // the only option it to loop through all sinks
    // and check for event names, TODO?

    auto sink = new Sink(
      eventNames,
      // Activate
      [](const ::Sink* sink) {
        const auto handler = EventHandler::GetSingleton();
        handler->ActivateSink<E>(sink, GetEventSourceScriptEvent<E>());
      },
      // Deactivate
      [](const ::Sink* sink) {
        const auto handler = EventHandler::GetSingleton();
        handler->DeactivateSink<E>(sink, GetEventSourceScriptEvent<E>());
      },
      // IsActive
      [](const ::Sink* sink) -> bool {
        const auto handler = EventHandler::GetSingleton();
        return handler->IsActiveSink(sink);
      });

    sinks.emplace(sink);
  }

  /**
   * @brief Create new sink instance and add it to sink set.
   * Registration via script event source.
   */
  template <class E>
  void AppendSink(std::vector<const char*> eventNames)
  {
    // should consider checking if sink exists
    // but since we store sink pointers
    // the only option it to loop through all sinks
    // and check for event names, TODO?

    auto sink = new Sink(
      eventNames,
      // Activate
      [](const ::Sink* sink) {
        const auto handler = EventHandler::GetSingleton();
        handler->ActivateSink<E>(sink);
      },
      // Deactivate
      [](const ::Sink* sink) {
        const auto handler = EventHandler::GetSingleton();
        handler->DeactivateSink<E>(sink);
      },
      // IsActive
      [](const ::Sink* sink) -> bool {
        const auto handler = EventHandler::GetSingleton();
        return handler->IsActiveSink(sink);
      });

    sinks.emplace(sink);
  }

  /**
   * @brief Create new sink instance and add it to sink set.
   * Registration via specific class as event source.
   */
  template <class T, class E>
  void AppendSink(std::vector<const char*> eventNames)
  {
    // should consider checking if sink exists
    // but since we store sink pointers
    // the only option it to loop through all sinks
    // and check for event names, TODO?

    auto sink = new Sink(
      eventNames,
      // Activate
      [](const ::Sink* sink) {
        const auto handler = EventHandler::GetSingleton();
        handler->ActivateSink<T, E>(sink);
      },
      // Deactivate
      [](const ::Sink* sink) {
        const auto handler = EventHandler::GetSingleton();
        handler->DeactivateSink<T, E>(sink);
      },
      // IsActive
      [](const ::Sink* sink) -> bool {
        const auto handler = EventHandler::GetSingleton();
        return handler->IsActiveSink(sink);
      });

    sinks.emplace(sink);
  }

  SinkSet sinks;
  SinkSet activeSinks;
};
