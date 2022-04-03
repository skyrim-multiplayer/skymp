#include "SendAnimationEvent.h"
#include "NullPointerException.h"

void SendAnimationEvent::Run(const std::vector<CallNative::AnySafe>& _args)
{
  if (_args.size() != 2)
    throw std::runtime_error("SendAnimationEvent expects 2 arguments");
  if (_args[0].index() != CallNative::GetIndexFor<CallNative::ObjectPtr>())
    throw std::runtime_error("Expected param1 to be an object");

  if (_args[1].index() != CallNative::GetIndexFor<std::string>())
    throw std::runtime_error("Expected param2 to be string");

  if (auto objPtr = std::get<CallNative::ObjectPtr>(_args[0])) {
    // why not cast to ObjectREFR right away?
    auto nativeObjPtr = (RE::TESForm*)objPtr->GetNativeObjectPtr();
    if (!nativeObjPtr)
      throw NullPointerException("nativeObjPtr");
    auto refr = nativeObjPtr->As<RE::TESObjectREFR>();

    if (!refr) {
      throw std::runtime_error(
        "Expected param1 to be ObjectReference but got " +
        std::string(objPtr->GetType()));
    }

    auto animEventName = CallNative::AnySafeToVariable(_args[1], false);
    refr->NotifyAnimationGraph(animEventName.GetString());
  }
}
