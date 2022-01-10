#pragma once
#include <RE/BGSLocation.h>
#include <RE/ScriptEventSourceHolder.h>
#include <RE/TESObjectCELL.h>
#include <RE/TESObjectREFR.h>

namespace TESEvents {

struct TESSpellCastEvent
{
public:
  TESSpellCastEvent();
  TESSpellCastEvent(RE::TESObjectREFR* caster, RE::FormID spell);
  ~TESSpellCastEvent() = default;

  RE::NiPointer<RE::TESObjectREFR> caster;
  RE::FormID spell;
};
static_assert(sizeof(TESSpellCastEvent) == 0x10);

struct TESOpenCloseEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::NiPointer<RE::TESObjectREFR> cause;
  bool isOpened;
};

struct TESQuestInitEvent
{
  RE::FormID questId;
};

struct TESQuestStartStopEvent
{
  RE::FormID questId;
  bool isStarted;
};

struct TESQuestStageEvent
{
  void* finishedCallback;
  RE::FormID questId;
  UInt16 stage;
  UInt8 unk;
  UInt8 pad;
};

struct TESQuestStageItemDoneEvent // not finished
{
  UInt32 stage;
  RE::FormID questId;
  void* one;   // ? pointer
  void* two;   // pointer?
  UInt32 flag; // 1, -1 ?
};

struct TESTriggerEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::NiPointer<RE::TESObjectREFR> cause;
};

struct TESTriggerEnterEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::NiPointer<RE::TESObjectREFR> cause;
};

struct TESTriggerLeaveEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  RE::NiPointer<RE::TESObjectREFR> cause;
};

struct TESSleepStartEvent
{
  float sleepStartTime;
  float desiredSleepEndTime;
};

struct TESSleepStopEvent
{
  bool isInterrupted;
};

struct TESCellAttachDetachEvent
{
  RE::NiPointer<RE::TESObjectREFR> reference;
  UInt8 action;
};

struct TESWaitStartEvent
{
  float waitStartTime;
  float desiredWaitEndTime;
};

struct TESActorLocationChangeEvent
{
  RE::NiPointer<RE::TESObjectREFR> actor;
  RE::BGSLocation* oldLoc;
  RE::BGSLocation* newLoc;
};

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

struct TESFurnitureEvent
{
public:
  enum class EventType : UInt32
  {
    kEnter = 0,
    kExit = 1
  };
  RE::NiPointer<RE::TESObjectREFR> actor;
  RE::NiPointer<RE::TESObjectREFR> target;
  EventType type;
};

struct TESMagicWardHitEvent
{
  enum class Status : UInt32
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
  enum class EventType : UInt32 // not sure
  {
    kStart = 0,
    kChange = 1,
    kEnd = 2
  };
  RE::NiPointer<RE::TESObjectREFR> actor;
  RE::FormID package;
  EventType type;
};

struct TESEnterBleedoutEvent
{
  RE::NiPointer<RE::TESObjectREFR> actor;
};

struct TESDestructionStageChangedEvent
{
  RE::NiPointer<RE::TESObjectREFR> target;
  UInt32 oldStage;
  UInt32 newStage;
};

struct TESSceneActionEvent
{
  RE::TESForm* reference;
  RE::FormID sceneId;
  RE::FormID referenceAliasID;
  RE::FormID questId;
  UInt32 action; // not sure, where enum?
};

struct TESSceneEvent
{
  RE::TESForm* reference; // what ref?
  RE::FormID sceneId;
};

struct TESPlayerBowShotEvent
{
  RE::FormID weaponId;
  RE::FormID ammoId;
  float power;
  bool isSunGazing;
};

struct TESFastTravelEndEvent
{
  float travelTimeGameHours;
};

struct TESObjectREFRTranslationEvent
{
  enum class EventType : UInt32
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
  UInt32 flag; // flag ? 0 - traphit, 1 - traphitend, 2 - traphitstart
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
  UInt32 flag;
};

}
