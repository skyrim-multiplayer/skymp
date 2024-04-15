#pragma once

#include "events/ActorKill.h"
#include "events/BooksRead.h"
#include "events/CriticalHit.h"
#include "events/DisarmedEvent.h"
#include "events/DragonSoulsGained.h"
#include "events/ItemHarvested.h"
#include "events/LevelIncrease.h"
#include "events/LocationDiscovery.h"
#include "events/ShoutAttack.h"
#include "events/SkillIncrease.h"
#include "events/SoulsTrapped.h"
#include "events/SpellsLearned.h"

namespace RE {
#ifdef SKYRIMSE
struct TESSpellCastEvent
{
public:
  TESSpellCastEvent()
    : TESSpellCastEvent(nullptr, static_cast<RE::FormID>(0))
  {
  }

  TESSpellCastEvent(RE::TESObjectREFR* object, RE::FormID spell)
    : object(object)
    , spell(spell)
  {
  }
  ~TESSpellCastEvent() = default;

  RE::NiPointer<RE::TESObjectREFR> object;
  RE::FormID spell;
};
static_assert(sizeof(TESSpellCastEvent) == 0x10);
#endif
struct TESQuestInitEvent
{
  RE::FormID questId;
};

#ifdef SKYRIMSE
struct TESQuestStartStopEvent
{
  RE::FormID formID;
  bool started;
  uint8_t unk05;
	uint16_t pad06;
};
#endif

#ifdef SKYRIMSE
struct TESQuestStageEvent
{
  void* finishedCallback;
  RE::FormID formID;
  uint16_t stage;
  uint8_t unk;
  uint8_t pad;
};
#endif

struct TESQuestStageItemDoneEvent // not finished
{
  uint32_t stage;
  RE::FormID questId;
  void* one;     // ? pointer
  void* two;     // pointer?
  uint32_t flag; // 1, -1 ?
};

struct TESTriggerEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::NiPointer<RE::TESObjectREFR> caster;
};

struct TESTriggerEnterEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::NiPointer<RE::TESObjectREFR> caster;
};

struct TESTriggerLeaveEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::NiPointer<RE::TESObjectREFR> caster;
};

struct TESSleepStartEvent
{
  float sleepStartTime;
  float desiredSleepEndTime;
};

#ifdef SKYRIMSE
struct TESSleepStopEvent
{
  bool interrupted;
};
#endif

#ifdef SKYRIMSE
struct TESCellAttachDetachEvent
{
  RE::NiPointer<RE::TESObjectREFR> reference;
  bool attached;
};
#endif

struct TESWaitStartEvent
{
  float waitStartTime;
  float desiredWaitEndTime;
};

#ifdef SKYRIMSE
struct TESActorLocationChangeEvent
{
  RE::NiPointer<RE::TESObjectREFR> actor;
  RE::BGSLocation* oldLoc;
  RE::BGSLocation* newLoc;
};
#endif

struct TESBookReadEvent
{
  RE::NiPointer<RE::TESObjectREFR> book;
};

struct TESSellEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::NiPointer<RE::TESObjectREFR> seller;
};

struct TESCellReadyToApplyDecalsEvent
{
  RE::NiPointer<RE::TESObjectCELL> cell;
};

struct TESMagicWardHitEvent
{
  enum class Status : uint32_t
  {
    kFriendly = 0,
    kAbsorbed = 1,
    kBroken = 2
  };
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::NiPointer<RE::TESObjectREFR> caster;
  RE::FormID spell;
  Status status;
};

struct TESPackageEvent
{
  enum class EventType : uint32_t // not sure
  {
    kStart = 0,
    kChange = 1,
    kEnd = 2
  };
  RE::NiPointer<RE::TESObjectREFR> actor;
  RE::FormID package;
  EventType type;
};

#ifdef SKYRIMSE
struct TESEnterBleedoutEvent
{
  RE::NiPointer<RE::TESObjectREFR> actor;
};
#endif

struct TESDestructionStageChangedEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  uint32_t oldStage;
  uint32_t newStage;
};

struct TESSceneActionEvent
{
  void* reference;
  RE::FormID sceneId;
  uint32_t actionIndex;
  RE::FormID questId;
  uint32_t actorAliasId;
};

struct TESSceneEvent
{
  RE::TESForm* reference; // what ref?
  RE::FormID sceneId;
};

#ifdef SKYRIMSE
struct TESPlayerBowShotEvent
{
  RE::FormID weapon;
  RE::FormID ammo;
  float shotPower;
  bool isSunGazing;
};
#endif

#ifdef SKYRIMSE
struct TESFastTravelEndEvent
{
  float fastTravelEndHours;
  uint32_t pad04;
};
#endif

struct TESObjectREFRTranslationEvent
{
  enum class EventType : uint32_t
  {
    kFailed = 0,
    kCompleted = 1,
    kAlmostCompleted = 2
  };
  RE::NiPointer<RE::TESObjectREFR> refr;
  EventType type;
};

struct TESTrapHitEvent // not finished
{
  RE::NiPointer<RE::TESObjectREFR> trap;
  RE::NiPointer<RE::TESObjectREFR> target;
  uint32_t flag; // flag ? 0 - traphit, 1 - traphitend, 2 - traphitstart
  float f1;
  float f2;
  float f3;
  float f4;
  float f5;
  float f6;
  float f7; // 120 damage
  float f8;
  float f9;
  float f10;
  float f11;
  float f12;
  float f13;
  float f14;
};

struct TESPerkEntryRunEvent
{
  RE::NiPointer<RE::TESObjectREFR> cause;
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::FormID perkId;
  uint32_t flag;
};

}
