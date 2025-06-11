#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct SetRaceMenuOpenMessage : public MessageBase<SetRaceMenuOpenMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char,
                           static_cast<char>(MsgType::SetRaceMenuOpen)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("open", open);
  }

  bool open = false;
};
