#include "EventsApi.h"

#include "GameEventSinks.h"
#include "InvalidArgumentException.h"
#include "NativeObject.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include "SkyrimPlatform.h"
#include "ThreadPoolWrapper.h"
#include "TickTask.h"
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>

#include <RE/ConsoleLog.h>

namespace {
enum class PatternType
{
  Exact,
  StartsWith,
  EndsWith
};

class Pattern
{
public:
  static Pattern Parse(const std::string& str)
  {
    auto count = std::count(str.begin(), str.end(), '*');
    if (count == 0) {
      return { PatternType::Exact, str };
    }
    if (count > 1) {
      throw std::runtime_error(
        "Patterns can contain only one '*' at the beginning/end of string");
    }

    auto pos = str.find('*');
    if (pos == 0) {
      return { PatternType::EndsWith,
               std::string(str.begin() + 1, str.end()) };
    }
    if (pos == str.size() - 1) {
      return { PatternType::StartsWith,
               std::string(str.begin(), str.end() - 1) };
    }
    throw std::runtime_error(
      "In patterns '*' must be at the beginning/end of string");
  }

  PatternType type;
  std::string str;
};

class Handler
{
public:
  Handler() = default;

  Handler(const JsValue& handler_, std::optional<double> minSelfId_,
          std::optional<double> maxSelfId_, std::optional<Pattern> pattern_)
    : enter(handler_.GetProperty("enter"))
    , leave(handler_.GetProperty("leave"))
    , minSelfId(minSelfId_)
    , maxSelfId(maxSelfId_)
    , pattern(pattern_)
  {
  }

  bool Matches(uint32_t selfId, const std::string& eventName)
  {
    if (minSelfId.has_value() && selfId < minSelfId.value()) {
      return false;
    }
    if (maxSelfId.has_value() && selfId > maxSelfId.value()) {
      return false;
    }
    if (pattern.has_value()) {
      switch (pattern->type) {
        case PatternType::Exact:
          return eventName == pattern->str;
        case PatternType::StartsWith:
          return eventName.size() >= pattern->str.size() &&
            !memcmp(eventName.data(), pattern->str.data(),
                    pattern->str.size());
        case PatternType::EndsWith:
          return eventName.size() >= pattern->str.size() &&
            !memcmp(eventName.data() +
                      (eventName.size() - pattern->str.size()),
                    pattern->str.data(), pattern->str.size());
      }
    }
    return true;
  }

  // PerThread structure is unique for each thread
  struct PerThread
  {
    JsValue storage, context;
    bool matchesCondition = false;
  };
  std::unordered_map<DWORD, PerThread> perThread;

  // Shared between threads
  const JsValue enter, leave;
  const std::optional<Pattern> pattern;
  const std::optional<double> minSelfId;
  const std::optional<double> maxSelfId;
};

class Hook
{
public:
  Hook(std::string hookName_, std::string eventNameVariableName_,
       std::optional<std::string> succeededVariableName_)
    : hookName(hookName_)
    , eventNameVariableName(eventNameVariableName_)
    , succeededVariableName(succeededVariableName_)
  {
  }

  // Chakra thread only
  uint32_t AddHandler(const Handler& handler)
  {
    if (addRemoveBlocker) {
      throw std::runtime_error("Trying to add hook inside hook context");
    }
    handlers.emplace(hCounter, handler);
    return hCounter++;
  }

  void RemoveHandler(const uint32_t& id)
  {
    if (addRemoveBlocker) {
      throw std::runtime_error("Trying to remove hook inside hook context");
    }
    handlers.erase(id);
  }

  // Thread-safe, but it isn't too useful actually
  std::string GetName() const { return hookName; }

  // Hooks are set on game functions that are being called from multiple
  // threads. So Enter/Leave methods are thread-safe, but all private methods
  // are for Chakra thread only

  void Enter(uint32_t selfId, std::string& eventName)
  {
    addRemoveBlocker++;
    DWORD owningThread = GetCurrentThreadId();

    if (hookName == "sendPapyrusEvent") {
      // If there are no handlers, do not do g_taskQueue
      bool anyMatch = false;
      for (auto& hp : handlers) {
        auto* h = &hp.second;
        if (h->Matches(selfId, eventName)) {
          anyMatch = true;
          break;
        }
      }
      if (!anyMatch) {
        return;
      }

      return SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
        std::string s = eventName;
        HandleEnter(owningThread, selfId, s);
      });
    }

    auto f = [&](int) {
      try {
        if (inProgressThreads.count(owningThread))
          throw std::runtime_error("'" + hookName + "' is already processing");
        inProgressThreads.insert(owningThread);
        HandleEnter(owningThread, selfId, eventName);
      } catch (std::exception& e) {
        auto err = std::string(e.what()) + " (while performing enter on '" +
          hookName + "')";
        SkyrimPlatform::GetSingleton().AddUpdateTask(
          [err] { throw std::runtime_error(err); });
      }
    };
    SkyrimPlatform::GetSingleton().PushAndWait(f);
    addRemoveBlocker--;
  }

  void Leave(bool succeeded)
  {
    addRemoveBlocker++;
    DWORD owningThread = GetCurrentThreadId();

    if (hookName == "sendPapyrusEvent") {
      return;
    }

    auto f = [&](int) {
      try {
        if (!inProgressThreads.count(owningThread))
          throw std::runtime_error("'" + hookName + "' is not processing");
        inProgressThreads.erase(owningThread);
        HandleLeave(owningThread, succeeded);

      } catch (std::exception& e) {
        std::string what = e.what();
        SkyrimPlatform::GetSingleton().AddUpdateTask([what] {
          throw std::runtime_error(what + " (in SendAnimationEventLeave)");
        });
      }
    };
    SkyrimPlatform::GetSingleton().PushAndWait(f);
    addRemoveBlocker--;
  }

private:
  void HandleEnter(DWORD owningThread, uint32_t selfId, std::string& eventName)
  {
    for (auto& hp : handlers) {
      auto* h = &hp.second;
      auto& perThread = h->perThread[owningThread];
      perThread.matchesCondition = h->Matches(selfId, eventName);
      if (!perThread.matchesCondition) {
        continue;
      }

      PrepareContext(perThread);
      ClearContextStorage(perThread);

      perThread.context.SetProperty("selfId", static_cast<double>(selfId));
      perThread.context.SetProperty(eventNameVariableName, eventName);
      h->enter.Call({ JsValue::Undefined(), perThread.context });

      eventName = static_cast<std::string>(
        perThread.context.GetProperty(eventNameVariableName));
    }
  }

  void PrepareContext(Handler::PerThread& h)
  {
    if (h.context.GetType() != JsValue::Type::Object) {
      h.context = JsValue::Object();
    }

    thread_local auto g_standardMap =
      JsValue::GlobalObject().GetProperty("Map");
    if (h.storage.GetType() != JsValue::Type::Object) {
      h.storage = g_standardMap.Constructor({ g_standardMap });
      h.context.SetProperty("storage", h.storage);
    }
  }

  void ClearContextStorage(Handler::PerThread& h)
  {
    thread_local auto g_standardMap =
      JsValue::GlobalObject().GetProperty("Map");
    thread_local auto g_clear =
      g_standardMap.GetProperty("prototype").GetProperty("clear");
    g_clear.Call({ h.storage });
  }

  void HandleLeave(DWORD owningThread, bool succeeded)
  {
    for (auto& hp : handlers) {
      auto* h = &hp.second;
      auto& perThread = h->perThread.at(owningThread);
      if (!perThread.matchesCondition) {
        continue;
      }

      PrepareContext(perThread);

      if (succeededVariableName.has_value()) {
        perThread.context.SetProperty(succeededVariableName.value(),
                                      JsValue::Bool(succeeded));
      }
      h->leave.Call({ JsValue::Undefined(), perThread.context });
      h->perThread.erase(owningThread);
    }
  }

  const std::string hookName;
  const std::string eventNameVariableName;
  const std::optional<std::string> succeededVariableName;
  std::set<DWORD> inProgressThreads;
  std::map<uint32_t, Handler> handlers;
  uint32_t hCounter = 0;
  std::atomic<int> addRemoveBlocker = 0;
};
}

struct EventsGlobalStatePersistent
{
  std::shared_ptr<GameEventSinks> gameEventSinks;
} g_persistent;

struct EventsGlobalState
{
  EventsGlobalState()
  {
    sendAnimationEvent.reset(
      new Hook("sendAnimationEvent", "animEventName", "animationSucceeded"));
    sendPapyrusEvent.reset(
      new Hook("sendPapyrusEvent", "papyrusEventName", std::nullopt));
  }

  using Callbacks = std::map<std::string, std::vector<JsValue>>;
  Callbacks callbacks;
  Callbacks callbacksOnce;
  std::shared_ptr<Hook> sendAnimationEvent;
  std::shared_ptr<Hook> sendPapyrusEvent;
} g;

std::vector<GameEventSinks::SinkClass> g_Sinks;

const std::map<std::string, GameEventSinks::SinkClass> eventSinksMap{
  { "tick", GameEventSinks::SinkClass::kNone },
  { "update", GameEventSinks::SinkClass::kNone },
  { "effectStart",
    GameEventSinks::SinkClass::kTESActiveEffectApplyRemoveEvent },
  { "effectFinish",
    GameEventSinks::SinkClass::kTESActiveEffectApplyRemoveEvent },
  { "magicEffectApply", GameEventSinks::SinkClass::kTESMagicEffectApplyEvent },
  { "equip", GameEventSinks::SinkClass::kTESEquipEvent },
  { "unequip", GameEventSinks::SinkClass::kTESEquipEvent },
  { "hit", GameEventSinks::SinkClass::kTESHitEvent },
  { "containerChanged", GameEventSinks::SinkClass::kTESContainerChangedEvent },
  { "deathStart", GameEventSinks::SinkClass::kTESDeathEvent },
  { "deathEnd", GameEventSinks::SinkClass::kTESDeathEvent },
  { "loadGame", GameEventSinks::SinkClass::kTESLoadGameEvent },
  { "combatState", GameEventSinks::SinkClass::kTESCombatEvent },
  { "reset", GameEventSinks::SinkClass::kTESResetEvent },
  { "scriptInit", GameEventSinks::SinkClass::kTESInitScriptEvent },
  { "trackedStats", GameEventSinks::SinkClass::kTESTrackedStatsEvent },
  { "uniqueIdChange", GameEventSinks::SinkClass::kTESUniqueIDChangeEvent },
  { "switchRaceComplete",
    GameEventSinks::SinkClass::kTESSwitchRaceCompleteEvent },
  { "cellFullyLoaded", GameEventSinks::SinkClass::kTESCellFullyLoadedEvent },
  { "cellAttach", GameEventSinks::SinkClass::kTESCellAttachDetachEvent },
  { "cellDetach", GameEventSinks::SinkClass::kTESCellAttachDetachEvent },
  { "grabRelease", GameEventSinks::SinkClass::kTESGrabReleaseEvent },
  { "lockChanged", GameEventSinks::SinkClass::kTESLockChangedEvent },
  { "moveAttachDetach", GameEventSinks::SinkClass::kTESMoveAttachDetachEvent },
  { "objectLoaded", GameEventSinks::SinkClass::kTESObjectLoadedEvent },
  { "waitStart", GameEventSinks::SinkClass::kTESWaitStartEvent },
  { "waitStop", GameEventSinks::SinkClass::kTESWaitStopEvent },
  { "activate", GameEventSinks::SinkClass::kTESActivateEvent },
  { "ipcMessage", GameEventSinks::SinkClass::kNone },
  { "menuOpen", GameEventSinks::SinkClass::kMenuOpenCloseEvent },
  { "menuClose", GameEventSinks::SinkClass::kMenuOpenCloseEvent },
  { "browserMessage", GameEventSinks::SinkClass::kNone },
  { "consoleMessage", GameEventSinks::SinkClass::kNone },
  { "spellCast", GameEventSinks::SinkClass::kTESSpellCastEvent },
  { "open", GameEventSinks::SinkClass::kTESOpenCloseEvent },
  { "close", GameEventSinks::SinkClass::kTESOpenCloseEvent },
  { "questInit", GameEventSinks::SinkClass::kTESQuestInitEvent },
  { "questStart", GameEventSinks::SinkClass::kTESQuestStartStopEvent },
  { "questStop", GameEventSinks::SinkClass::kTESQuestStartStopEvent },
  { "questStage", GameEventSinks::SinkClass::kTESQuestStageEvent },
  { "trigger", GameEventSinks::SinkClass::kTESTriggerEvent },
  { "triggerEnter", GameEventSinks::SinkClass::kTESTriggerEnterEvent },
  { "triggerLeave", GameEventSinks::SinkClass::kTESTriggerLeaveEvent },
  { "sleepStart", GameEventSinks::SinkClass::kTESSleepStartEvent },
  { "sleepStop", GameEventSinks::SinkClass::kTESSleepStopEvent },
  { "locationChanged",
    GameEventSinks::SinkClass::kTESActorLocationChangeEvent },
  { "bookRead", GameEventSinks::SinkClass::kTESBookReadEvent },
  { "sell", GameEventSinks::SinkClass::kTESSellEvent },
  { "furnitureEnter", GameEventSinks::SinkClass::kTESFurnitureEvent },
  { "furnitureExit", GameEventSinks::SinkClass::kTESFurnitureEvent },
  { "wardHit", GameEventSinks::SinkClass::kTESMagicWardHitEvent },
  { "packageStart", GameEventSinks::SinkClass::kTESPackageEvent },
  { "packageChange", GameEventSinks::SinkClass::kTESPackageEvent },
  { "packageEnd", GameEventSinks::SinkClass::kTESPackageEvent },
  { "enterBleedout", GameEventSinks::SinkClass::kTESEnterBleedoutEvent },
  { "destructionStageChanged",
    GameEventSinks::SinkClass::kTESDestructionStageChangedEvent },
  { "sceneAction", GameEventSinks::SinkClass::kTESSceneActionEvent },
  { "playerBowShot", GameEventSinks::SinkClass::kTESPlayerBowShotEvent },
  { "fastTravelEnd", GameEventSinks::SinkClass::kTESFastTravelEndEvent },
  { "perkEntryRun", GameEventSinks::SinkClass::kTESPerkEntryRunEvent },
  { "translationFailed",
    GameEventSinks::SinkClass::kTESObjectREFRTranslationEvent },
  { "translationAlmostCompleted",
    GameEventSinks::SinkClass::kTESObjectREFRTranslationEvent },
  { "translationCompleted",
    GameEventSinks::SinkClass::kTESObjectREFRTranslationEvent },
  { "actionWeaponSwing", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionBeginDraw", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionEndDraw", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionBowDraw", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionBowRelease", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionBeginSheathe", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionEndSheathe", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionSpellCast", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionSpellFire", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionVoiceCast", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "actionVoiceFire", GameEventSinks::SinkClass::kSKSEActionEvent },
  { "cameraStateChanged", GameEventSinks::SinkClass::kSKSECameraEvent },
  { "crosshairRefChanged", GameEventSinks::SinkClass::kSKSECrosshairRefEvent },
  { "niNodeUpdate", GameEventSinks::SinkClass::kSKSENiNodeUpdateEvent },
  { "modEvent", GameEventSinks::SinkClass::kSKSEModCallbackEvent },
  { "positionPlayer", GameEventSinks::SinkClass::kPositionPlayerEvent },
  { "footstep", GameEventSinks::SinkClass::kBGSFootstepEvent },
  { "buttonEvent", GameEventSinks::SinkClass::kInputEvent },
  { "mouseMove", GameEventSinks::SinkClass::kInputEvent },
  { "thumbstickEvent", GameEventSinks::SinkClass::kInputEvent },
  { "kinectEvent", GameEventSinks::SinkClass::kInputEvent },
  { "deviceConnect", GameEventSinks::SinkClass::kInputEvent }
};

void EventsApi::RemoveSinks()
{
  for (int i = 0; i < g_Sinks.size(); ++i) {
    GameEventSinks::GetSingleton()->EventSinksAction(
      g_Sinks[i], GameEventSinks::SinkAction::kRemove);
  }
  g_Sinks.clear();
}

void EventsApi::AddSinks()
{
  for (int i = 0; i < g_Sinks.size(); ++i) {
    GameEventSinks::GetSingleton()->EventSinksAction(
      g_Sinks[i], GameEventSinks::SinkAction::kAdd);
  }
}

class IpcCallbackData
{
public:
  IpcCallbackData() = default;
  IpcCallbackData(std::string systemName_,
                  EventsApi::IpcMessageCallback callback_, void* state_)
    : systemName(systemName_)
    , callback(callback_)
    , state(state_)
  {
  }

  friend bool operator==(const IpcCallbackData& lhs,
                         const IpcCallbackData& rhs)
  {
    return std::make_tuple(lhs.systemName, lhs.callback, lhs.state) ==
      std::make_tuple(rhs.systemName, rhs.callback, rhs.state);
  }

  std::string systemName;
  EventsApi::IpcMessageCallback callback;
  void* state = nullptr;
};

struct IpcShare
{
  std::recursive_mutex m;
  std::vector<IpcCallbackData> ipcCallbacks;
} g_ipcShare;

std::atomic<uint32_t> g_chakraThreadId = 0;

namespace {
void CallCalbacks(const char* eventName, const std::vector<JsValue>& arguments,
                  bool isOnce = false)
{
  EventsGlobalState::Callbacks callbacks =
    isOnce ? g.callbacksOnce : g.callbacks;

  if (isOnce)
    g.callbacksOnce[eventName].clear();

  for (auto& f : callbacks[eventName]) {
    f.Call(arguments);
  }
}
}

void EventsApi::SendEvent(const char* eventName,
                          const std::vector<JsValue>& arguments)
{
  CallCalbacks(eventName, arguments);
  CallCalbacks(eventName, arguments, true);
}

void EventsApi::Clear()
{
  g_chakraThreadId = GetCurrentThreadId();
  g = {};
  RemoveSinks();
}

void EventsApi::SendAnimationEventEnter(uint32_t selfId,
                                        std::string& animEventName) noexcept
{
  g.sendAnimationEvent->Enter(selfId, animEventName);
}

void EventsApi::SendAnimationEventLeave(bool animationSucceeded) noexcept
{
  g.sendAnimationEvent->Leave(animationSucceeded);
}

void EventsApi::SendPapyrusEventEnter(uint32_t selfId,
                                      std::string& papyrusEventName) noexcept
{
  g.sendPapyrusEvent->Enter(selfId, papyrusEventName);
}

void EventsApi::SendPapyrusEventLeave() noexcept
{
  g.sendPapyrusEvent->Leave(true);
}

namespace {
JsValue CreateHookApi(std::shared_ptr<Hook> hookInfo)
{
  auto hook = JsValue::Object();
  hook.SetProperty(
    "add", JsValue::Function([hookInfo](const JsFunctionArguments& args) {
      auto handlerObj = args[1];

      std::optional<double> minSelfId;
      if (args[2].GetType() == JsValue::Type::Number) {
        minSelfId = static_cast<double>(args[2]);
      }

      std::optional<double> maxSelfId;
      if (args[3].GetType() == JsValue::Type::Number) {
        maxSelfId = static_cast<double>(args[3]);
      }

      std::optional<Pattern> pattern;
      if (args[4].GetType() == JsValue::Type::String) {
        pattern = Pattern::Parse(static_cast<std::string>(args[4]));
      }

      Handler handler(handlerObj, minSelfId, maxSelfId, pattern);
      uint32_t id = hookInfo->AddHandler(handler);

      return JsValue((int)id);
    }));

  hook.SetProperty(
    "remove", JsValue::Function([hookInfo](const JsFunctionArguments& args) {
      uint32_t toRemove = static_cast<int>(args[1]);
      hookInfo->RemoveHandler(toRemove);
      return JsValue::Undefined();
    }));
  return hook;
}
}

JsValue EventsApi::GetHooks()
{
  auto res = JsValue::Object();
  for (auto& hook : { g.sendAnimationEvent, g.sendPapyrusEvent }) {
    res.SetProperty(hook->GetName(), CreateHookApi(hook));
  }
  return res;
}

uint32_t EventsApi::IpcSubscribe(const char* systemName,
                                 IpcMessageCallback callback, void* state)
{
  // Maybe they decide calling IpcSubscribe from multiple threads...
  std::lock_guard l(g_ipcShare.m);

  IpcCallbackData ipcCallbackData(systemName, callback, state);

  auto it = std::find(g_ipcShare.ipcCallbacks.begin(),
                      g_ipcShare.ipcCallbacks.end(), IpcCallbackData());
  if (it == g_ipcShare.ipcCallbacks.end()) {
    g_ipcShare.ipcCallbacks.push_back(ipcCallbackData);
    return g_ipcShare.ipcCallbacks.size() - 1;
  } else {
    *it = ipcCallbackData;
    return static_cast<uint32_t>(it - g_ipcShare.ipcCallbacks.begin());
  }
}

void EventsApi::IpcUnsubscribe(uint32_t subscriptionId)
{
  std::lock_guard l(g_ipcShare.m);
  if (g_ipcShare.ipcCallbacks.size() > subscriptionId) {
    g_ipcShare.ipcCallbacks[subscriptionId] = IpcCallbackData();
  }
  // TODO: pop_back for empty subscriptions?
}

void EventsApi::IpcSend(const char* systemName, const uint8_t* data,
                        uint32_t length)
{
  const DWORD currentThreadId = GetCurrentThreadId();
  if (currentThreadId != g_chakraThreadId) {
    assert(0 && "IpcSend is only available in Chakra thread");
    return;
  }

  auto typedArray = JsValue::Uint8Array(length);
  memcpy(typedArray.GetTypedArrayData(), data, length);

  auto ipcMessageEvent = JsValue::Object();
  ipcMessageEvent.SetProperty("sourceSystemName", systemName);
  ipcMessageEvent.SetProperty("message", typedArray);
  SendEvent("ipcMessage", { JsValue::Undefined(), ipcMessageEvent });
}

void EventsApi::SendConsoleMsgEvent(const char* msg_)
{
  std::string msg(msg_);
  SkyrimPlatform::GetSingleton().AddTickTask([=] {
    auto obj = JsValue::Object();
    obj.SetProperty("message", JsValue::String(msg));
    EventsApi::SendEvent("consoleMessage", { JsValue::Undefined(), obj });
  });
}

void EventsApi::SendMenuOpen(const char* menuName)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    obj.SetProperty("name", JsValue::String(menuName));

    SendEvent("menuOpen", { JsValue::Undefined(), obj });
  });
}

void EventsApi::SendMenuClose(const char* menuName)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    obj.SetProperty("name", JsValue::String(menuName));

    SendEvent("menuClose", { JsValue::Undefined(), obj });
  });
}

void EventsApi::SendButtonEvent(uint32_t device, uint32_t code,
                                std::string& userEventName, float value,
                                float heldDownSecs, bool isPressed, bool isUp,
                                bool isDown, bool isHeld, bool isRepeating)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    obj.SetProperty("device", JsValue::Double(device));
    obj.SetProperty("code", JsValue::Double(code));
    obj.SetProperty("userEventName", JsValue::String(userEventName));
    obj.SetProperty("value", JsValue::Double(value));
    obj.SetProperty("heldDuration", JsValue::Double(heldDownSecs));
    obj.SetProperty("isPressed", JsValue::Bool(isPressed));
    obj.SetProperty("isUp", JsValue::Bool(isUp));
    obj.SetProperty("isDown", JsValue::Bool(isDown));
    obj.SetProperty("isHeld", JsValue::Bool(isHeld));
    obj.SetProperty("isRepeating", JsValue::Bool(isRepeating));

    SendEvent("buttonEvent", { JsValue::Undefined(), obj });
  });
}

void EventsApi::SendMouseMoveEvent(uint32_t device, uint32_t code,
                                   std::string& userEventName, double inputX,
                                   double inputY)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    obj.SetProperty("device", JsValue::Double(device));
    obj.SetProperty("code", JsValue::Double(code));
    obj.SetProperty("userEventName", JsValue::String(userEventName));
    obj.SetProperty("inputX", JsValue::Double(inputX));
    obj.SetProperty("inputY", JsValue::Double(inputY));

    SendEvent("mouseMove", { JsValue::Undefined(), obj });
  });
}

void EventsApi::SendThumbstickEvent(uint32_t device, uint32_t code,
                                    std::string& userEventName, float inputX,
                                    float inputY, bool isLeft, bool isRight)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    obj.SetProperty("device", JsValue::Double(device));
    obj.SetProperty("code", JsValue::Double(code));
    obj.SetProperty("userEventName", JsValue::String(userEventName));
    obj.SetProperty("inputX", JsValue::Double(inputX));
    obj.SetProperty("inputY", JsValue::Double(inputY));
    obj.SetProperty("isLeft", JsValue::Bool(isLeft));
    obj.SetProperty("isRight", JsValue::Bool(isRight));

    SendEvent("thumbstickEvent", { JsValue::Undefined(), obj });
  });
}

void EventsApi::SendKinectEvent(uint32_t device, uint32_t code,
                                std::string& userEventName, std::string& heard)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    obj.SetProperty("device", JsValue::Double(device));
    obj.SetProperty("code", JsValue::Double(code));
    obj.SetProperty("userEventName", JsValue::String(userEventName));
    obj.SetProperty("heard", JsValue::String(heard));

    SendEvent("kinectEvent", { JsValue::Undefined(), obj });
  });
}

void EventsApi::SendDeviceConnectEvent(uint32_t device, bool isConnected)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    obj.SetProperty("device", JsValue::Double(device));
    obj.SetProperty("isConnected", JsValue::Bool(isConnected));

    SendEvent("deviceConnect", { JsValue::Undefined(), obj });
  });
}

namespace {
JsValue AddCallback(const JsFunctionArguments& args, bool isOnce = false)
{
  if (!g_persistent.gameEventSinks) {
    g_persistent.gameEventSinks = std::make_shared<GameEventSinks>();
  }

  auto eventName = args[1].ToString();
  auto callback = args[2];

  if (eventSinksMap.count(eventName) == 0) {
    throw InvalidArgumentException("eventName", eventName);
  }

  if (std::find(g_Sinks.begin(), g_Sinks.end(),
                eventSinksMap.find(eventName)->second) == g_Sinks.end()) {
    g_Sinks.push_back(eventSinksMap.find(eventName)->second);
  }

  isOnce ? g.callbacksOnce[eventName].push_back(callback)
         : g.callbacks[eventName].push_back(callback);
  return JsValue::Undefined();
}
}

JsValue EventsApi::On(const JsFunctionArguments& args)
{
  return AddCallback(args);
}

JsValue EventsApi::Once(const JsFunctionArguments& args)
{
  return AddCallback(args, true);
}

JsValue EventsApi::SendIpcMessage(const JsFunctionArguments& args)
{
  auto targetSystemName = static_cast<std::string>(args[1]);
  auto message = args[2].GetArrayBufferData();
  auto messageLength = args[2].GetArrayBufferLength();

  if (!message || messageLength == 0) {
    throw std::runtime_error(
      "sendIpcMessage expects a valid ArrayBuffer instance");
  }

  std::vector<IpcCallbackData> callbacks;
  {
    std::lock_guard l(g_ipcShare.m);
    for (auto& callbackData : g_ipcShare.ipcCallbacks) {
      if (callbackData.systemName == targetSystemName) {
        callbacks.push_back(callbackData);
      }
    }
  }

  // Want to call callbacks with g_ipcShare.m unlocked
  for (auto& callbackData : callbacks) {
    if (callbackData.callback) {
      callbackData.callback(reinterpret_cast<uint8_t*>(message), messageLength,
                            callbackData.state);
    }
  }

  return JsValue::Undefined();
}
