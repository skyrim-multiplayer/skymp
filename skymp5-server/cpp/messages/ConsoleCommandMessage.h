#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <optional>
#include <type_traits>

struct ConsoleCommandMessage : public MessageBase<ConsoleCommandMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::ConsoleCommand)>{};

  struct Data
  {
    template <class Archive>
    void Serialize(Archive& archive)
    {
      archive.Serialize("t", kMsgType)
        .Serialize("commandName", commandName)
        .Serialize("args", args);
    }

    std::string commandName;
    std::vector<std::variant<int64_t, std::string>> args;
  };

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("data", data);
  }

  Data data;
};
