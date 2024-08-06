#pragma once
#include "MessageBase.h"
#include "MsgType.h"

#include "ChangeValuesMessage.h"
#include "TeleportMessage.h"
#include "UpdatePropertyMessage.h"

#include <optional>
#include <type_traits>

struct DeathStateContainerMessage
  : public MessageBase<DeathStateContainerMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char,
                           static_cast<char>(MsgType::DeathStateContainer)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("tTeleport", tTeleport)
      .Serialize("tChangeValues", tChangeValues)
      .Serialize("tIsDead", tIsDead);
  }

  std::optional<TeleportMessage> tTeleport;
  std::optional<ChangeValuesMessage> tChangeValues;
  std::optional<UpdatePropertyMessage> tIsDead;
};
