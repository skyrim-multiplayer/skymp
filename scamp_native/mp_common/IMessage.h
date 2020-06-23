#pragma once
#include "Networking.h"
#include "ServerState.h"
#include <simdjson.h>

class IMessage
{
public:
  virtual ~IMessage() = default;
  virtual void Set(const simdjson::dom::element& msg,
                   Networking::UserId userId_) = 0;
  virtual CachingRequest OnReceive(ServerState& st) = 0;
  virtual void OnCacheReady(ServerState& st){};
};