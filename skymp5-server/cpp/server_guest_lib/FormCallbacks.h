#pragma once
#include "MessageBase.h"
#include <functional>

class MpObjectReference;
class MpActor;

class FormCallbacks
{
public:
  using SubscribeCallback = std::function<void(MpObjectReference* emitter,
                                               MpObjectReference* listener)>;
  using SendToUserFn = std::function<void(
    MpActor* actor, const IMessageBase& message, bool reliable)>;

  // TODO: use MessageBase instead of raw data
  using SendToUserDeferredFn =
    std::function<void(MpActor* actor, const void* data, size_t size,
                       bool reliable, int deferredChannelId)>;

  SubscribeCallback subscribe, unsubscribe;
  SendToUserFn sendToUser;
  SendToUserDeferredFn sendToUserDeferred;

  static FormCallbacks DoNothing()
  {
    return { [](auto, auto) {}, [](auto, auto) {}, [](auto, auto&, auto) {},
             [](auto, auto, auto, auto, auto) {} };
  }
};
