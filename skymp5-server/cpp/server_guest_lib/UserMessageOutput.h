#pragma once
#include "IMessageOutput.h"
#include "NetworkingInterface.h"

class UserMessageOutput : public IMessageOutput
{
public:
  UserMessageOutput(Networking::ISendTarget& sendTarget_,
                    Networking::UserId userId_)
    : sendTarget(sendTarget_)
    , userId(userId_)
  {
  }

  void Send(const uint8_t* data, size_t length, bool reliable) override
  {
    sendTarget.Send(userId, data, length, reliable);
  }

private:
  Networking::ISendTarget& sendTarget;
  Networking::UserId userId;
};
