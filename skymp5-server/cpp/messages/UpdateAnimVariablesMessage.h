#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include "UpdateAnimVariablesMessage.h" // ActorAnimationVariables
#include <type_traits>

struct UpdateAnimVariablesMessageMsgData
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("actorRemoteId", actorRemoteId)
      .Serialize("actorAnimationVariables", actorAnimationVariables);
  }

  uint32_t actorRemoteId = 0;
  ActorAnimationVariables actorAnimationVariables;
};

struct UpdateAnimVariablesMessage
  : public MessageBase<UpdateAnimVariablesMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char,
                           static_cast<char>(MsgType::UpdateAnimVariables)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("data", data);
  }

  UpdateAnimVariablesMessageMsgData data;
};
