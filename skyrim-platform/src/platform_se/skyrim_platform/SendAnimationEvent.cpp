#include "SendAnimationEvent.h"
#include "NullPointerException.h"
#include <RE/BSAnimationGraphChannel.h>
#include <RE/BSAnimationGraphManager.h>
#include <RE/IAnimationGraphManagerHolder.h>
#include <RE/UserEvents.h>
#include <skse64/GameRTTI.h>
#include <skse64/GameReferences.h>
#include <skse64/PapyrusForm.h>

void SendAnimationEvent::Run(const std::vector<CallNative::AnySafe>& _args)
{
  if (_args.size() != 2)
    throw std::runtime_error("SendAnimationEvent expects 2 arguments");
  if (_args[0].index() != CallNative::GetIndexFor<CallNative::ObjectPtr>())
    throw std::runtime_error("Expected param1 to be an object");

  if (_args[1].index() != CallNative::GetIndexFor<std::string>())
    throw std::runtime_error("Expected param2 to be string");

  if (auto objPtr = std::get<CallNative::ObjectPtr>(_args[0])) {
    auto nativeObjPtr = (RE::TESForm*)objPtr->GetNativeObjectPtr();
    if (!nativeObjPtr)
      throw NullPointerException("nativeObjPtr");
    auto refr = DYNAMIC_CAST(nativeObjPtr, TESForm, TESObjectREFR);
    if (!refr) {
      throw std::runtime_error(
        "Expected param1 to be ObjectReference but got " +
        std::string(objPtr->GetType()));
    }

    auto animEventName = CallNative::AnySafeToVariable(_args[1], false);
    auto graphManagerHolder =
      reinterpret_cast<RE::IAnimationGraphManagerHolder*>(
        &refr->animGraphHolder);

    graphManagerHolder->NotifyAnimationGraph(animEventName.GetString());
  }
}