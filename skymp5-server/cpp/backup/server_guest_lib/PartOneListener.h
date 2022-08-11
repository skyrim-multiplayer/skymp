#pragma once
#include "Networking.h"
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
  virtual bool OnMpApiEvent(const char* eventName,
                            std::optional<simdjson::dom::element> args,
                            std::optional<uint32_t> formId) = 0;
};
