#pragma once
#include <functional>

class MpObjectReference;
class MpActor;

class FormCallbacks
{
public:
  using SubscribeCallback = std::function<void(MpObjectReference* emitter,
                                               MpObjectReference* listener)>;
  using SendToUserFn = std::function<void(MpActor* actor, const void* data,
                                          size_t size, bool reliable)>;

  SubscribeCallback subscribe, unsubscribe;
  SendToUserFn sendToUser;

  static FormCallbacks DoNothing()
  {
    return { [](auto, auto) {}, [](auto, auto) {},
             [](auto, auto, auto, auto) {} };
  }
};
