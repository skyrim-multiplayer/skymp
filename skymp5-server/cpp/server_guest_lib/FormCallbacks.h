#pragma once
#include "Aliases.h"
#include <functional>

class MpObjectReference;
class MpActor;

class FormCallbacks
{
public:
  using SubscribeCallback = std::function<void(MpObjectReference* emitter,
                                               MpObjectReference* listener)>;
  // entity or formId
  using SendToUserFn = std::function<void(entity_t entity, const void* data,
                                          size_t size, bool reliable)>;

public:
  static FormCallbacks DoNothing()
  {
    return { [](auto, auto) {}, [](auto, auto) {},
             [](auto, auto, auto, auto) {} };
  }

public:
  SubscribeCallback subscribe, unsubscribe;
  SendToUserFn sendToUser;
};
