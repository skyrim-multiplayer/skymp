#pragma once
#include <nlohmann/json_fwd.hpp>
#include <slikenet/types.h>

#include "archives/BitStreamInputArchive.h"
#include "archives/BitStreamOutputArchive.h"
#include "archives/JsonInputArchive.h"
#include "archives/JsonOutputArchive.h"

class IMessageBase
{
public:
  virtual ~IMessageBase() = default;

  virtual void WriteBinary(SLNet::BitStream& stream) const = 0;
  virtual void ReadBinary(SLNet::BitStream& stream) = 0;

  virtual void WriteJson(nlohmann::json& json) const = 0;
  virtual void ReadJson(const nlohmann::json& json) = 0;
};

template <class Message>
class MessageBase : public IMessageBase
{
public:
  void WriteBinary(SLNet::BitStream& stream) const override
  {
    BitStreamOutputArchive archive(stream);
    AsMessage().Serialize(archive);
  }

  void ReadBinary(SLNet::BitStream& stream) override
  {
    BitStreamInputArchive archive(stream);
    AsMessage().Serialize(archive);
  }

  void WriteJson(nlohmann::json& json) const override
  {
    JsonOutputArchive archive;
    AsMessage().Serialize(archive);
    json = std::move(archive.j);
  }

  void ReadJson(const nlohmann::json& json) override
  {
    JsonInputArchive archive(json);
    AsMessage().Serialize(archive);
  }

private:
  Message& AsMessage() const
  {
    return *const_cast<Message*>(reinterpret_cast<const Message*>(this));
  }
};
