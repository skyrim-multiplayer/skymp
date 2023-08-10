#pragma once

#include <CommonLibSSE-NG/include/RE/B/BSCoreTypes.h>
#include <CommonLibSSE-NG/include/RE/N/NiSmartPointer.h>
#include <CommonLibSSE-NG/include/RE/T/TESObjectREFR.h>
#include <cstdint>

namespace RE {

struct TESQuestInitEvent
{
  FormID questId{ 0 };
};

struct TESQuestStageItemDoneEvent // not finished
{
  uint32_t stage;
  FormID questId;
  void* one;     // ? pointer
  void* two;     // pointer?
  uint32_t flag; // 1, -1 ?
};

struct TESTriggerEvent
{
  NiPointer<TESObjectREFR> target;
  NiPointer<TESObjectREFR> caster;
};

struct TESTriggerEnterEvent
{
  NiPointer<TESObjectREFR> target;
  NiPointer<TESObjectREFR> caster;
};

struct TESTriggerLeaveEvent
{
  NiPointer<TESObjectREFR> target;
  NiPointer<TESObjectREFR> caster;
};

struct TESSleepStartEvent
{
  float sleepStartTime;
  float desiredSleepEndTime;
};

struct TESWaitStartEvent
{
  float waitStartTime;
  float desiredWaitEndTime;
};

struct TESBookReadEvent
{
  NiPointer<TESObjectREFR> book;
};

struct TESSellEvent
{
  NiPointer<TESObjectREFR> target;
  NiPointer<TESObjectREFR> seller;
};

struct TESCellReadyToApplyDecalsEvent
{
  NiPointer<TESObjectCELL> cell;
};

struct TESMagicWardHitEvent
{
  enum class Status : uint32_t
  {
    kFriendly = 0,
    kAbsorbed = 1,
    kBroken = 2
  };
  NiPointer<TESObjectREFR> target;
  NiPointer<TESObjectREFR> caster;
  FormID spell;
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

  NiPointer<TESObjectREFR> actor;
  FormID package;
  EventType type;
};

struct TESDestructionStageChangedEvent
{
  NiPointer<TESObjectREFR> target;
  uint32_t oldStage;
  uint32_t newStage;
};

struct TESSceneActionEvent
{
  void* reference;
  FormID sceneId;
  uint32_t actionIndex;
  FormID questId;
  uint32_t actorAliasId;
};

struct TESSceneEvent
{
  TESForm* reference; // what ref?
  FormID sceneId;
};

struct TESObjectREFRTranslationEvent
{
  enum class EventType : uint32_t
  {
    kFailed = 0,
    kCompleted = 1,
    kAlmostCompleted = 2
  };
  NiPointer<TESObjectREFR> refr;
  EventType type;
};

struct TESTrapHitEvent // not finished
{
  NiPointer<TESObjectREFR> trap;
  NiPointer<TESObjectREFR> target;
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
  NiPointer<TESObjectREFR> cause;
  NiPointer<TESObjectREFR> target;
  FormID perkId;
  uint32_t flag;
};

}
