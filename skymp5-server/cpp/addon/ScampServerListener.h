#pragma once
#include "PartOne.h"

class ScampServer;

class ScampServerListener : public PartOne::Listener
{
public:
  explicit ScampServerListener(ScampServer& server_);

  void OnConnect(Networking::UserId userId) override;

  void OnDisconnect(Networking::UserId userId) override;

  void OnCustomPacket(Networking::UserId userId,
                      const simdjson::dom::element& content) override;

  bool OnMpApiEvent(const char* eventName,
                    std::optional<simdjson::dom::element> args,
                    std::optional<uint32_t> formId) override;

private:
  ScampServer& server;
};
