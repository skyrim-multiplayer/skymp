#pragma once
#include "Networking.h"
#include "gamemode_events/GameModeEvent.h"
#include <optional>
#include <simdjson.h>

class PartOneListener
{
public:
  virtual ~PartOneListener() = default;
  virtual void OnConnect(Networking::UserId userId) = 0;
  virtual void OnDisconnect(Networking::UserId userId) = 0;
  virtual void OnCustomPacket(Networking::UserId userId,
                              const simdjson::dom::element& content) = 0;
  virtual bool OnMpApiEvent(const GameModeEvent& event) = 0;
};
