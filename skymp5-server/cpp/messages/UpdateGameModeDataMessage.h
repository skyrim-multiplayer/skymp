#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <array>
#include <type_traits>

struct GamemodeValuePair
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("name", name).Serialize("content", content);
  }

  std::string name;
  std::string content;
};

struct UpdateGameModeDataMessage
  : public MessageBase<UpdateGameModeDataMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char,
                           static_cast<char>(MsgType::UpdateGameModeData)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("eventSources", eventSources)
      .Serialize("updateOwnerFunctions", updateOwnerFunctions)
      .Serialize("updateNeighborFunctions", updateNeighborFunctions);
  }

  std::vector<GamemodeValuePair> eventSources;
  std::vector<GamemodeValuePair> updateOwnerFunctions;
  std::vector<GamemodeValuePair> updateNeighborFunctions;
};
