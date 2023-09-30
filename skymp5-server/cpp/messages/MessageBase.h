#pragma once
#include <nlohmann/json_fwd.hpp>
#include <slikenet/types.h>

struct MessageBase
{
  virtual ~MessageBase();

  virtual void WriteBinary(SLNet::BitStream& stream) const = 0;
  virtual void ReadBinary(SLNet::BitStream& stream) = 0;

  virtual void WriteJson(nlohmann::json& json) const = 0;
  virtual void ReadJson(const nlohmann::json& json) = 0;
};
