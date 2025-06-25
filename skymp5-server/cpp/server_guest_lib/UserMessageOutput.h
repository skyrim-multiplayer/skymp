#pragma once
#include "IMessageOutput.h"
#include "MessageSerializerFactory.h"
#include "NetworkingInterface.h"

class UserMessageOutput : public IMessageOutput
{
public:
  UserMessageOutput(MessageSerializer& messageSerializer_,
                    Networking::ISendTarget& sendTarget_,
                    Networking::UserId userId_)
    : messageSerializer(messageSerializer_)
    , sendTarget(sendTarget_)
    , userId(userId_)
  {
  }

  void Send(const IMessageBase& message, bool reliable) override
  {
    SLNet::BitStream stream;
    messageSerializer.Serialize(message, stream);
    sendTarget.Send(userId,
                    reinterpret_cast<Networking::PacketData>(stream.GetData()),
                    stream.GetNumberOfBytesUsed(), reliable);
  }

private:
  Networking::ISendTarget& sendTarget;
  Networking::UserId userId;
  MessageSerializer& messageSerializer;
};
