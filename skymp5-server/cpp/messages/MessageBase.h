#pragma once
#include <nlohmann/json_fwd.hpp>
#include <slikenet/types.h>

struct IMessageBase
{
  virtual ~IMessageBase();

  virtual char GetHeaderByte() const noexcept = 0;
  virtual char GetMsgType() const noexcept = 0;

  virtual void WriteBinary(SLNet::BitStream& stream) const = 0;
  virtual void ReadBinary(SLNet::BitStream& stream) = 0;

  virtual void WriteJson(nlohmann::json& json) const = 0;
  virtual void ReadJson(const nlohmann::json& json) = 0;
};

template <class Message>
struct MessageBase : public IMessageBase
{
  char GetHeaderByte() const noexcept override { return Message::kHeaderByte; }
  char GetMsgType() const noexcept override { return Message::kMsgType; }
};
