#pragma once
#include "MessageBase.h"
#include "NetworkingInterface.h"
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

  using SendToUserDeferredFn = std::function<void(
    MpActor* actor, const IMessageBase& message, bool reliable,
    int deferredChannelId, bool overwritePreviousChannelMessages)>;

  using GetUserIdFn = std::function<Networking::UserId(MpActor* actor)>;

  SubscribeCallback subscribe, unsubscribe;
  SendToUserFn sendToUser;
  SendToUserDeferredFn sendToUserDeferred;
  GetUserIdFn getUserId;

  static FormCallbacks DoNothing()
  {
    return { [](auto, auto) {}, [](auto, auto) {}, [](auto, auto&, auto) {},
             [](auto, auto&, auto, auto, auto) {},
             [](auto) { return Networking::InvalidUserId; } };
  }
};
