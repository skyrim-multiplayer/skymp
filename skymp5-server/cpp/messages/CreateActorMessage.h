#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct CreateActorMessage : public MessageBase<CreateActorMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::CreateActor)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    // TODO
  }

  // TODO
};
