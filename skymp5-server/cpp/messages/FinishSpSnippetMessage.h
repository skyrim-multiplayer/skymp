#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct FinishSpSnippetMessage : public MessageBase<FinishSpSnippetMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char,
                           static_cast<char>(MsgType::FinishSpSnippet)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive
      .Serialize("t", kMsgType)
      //.Serialize("returnValue", returnValue)
      .Serialize("snippetIdx", snippetIdx);
  }

  std::optional<std::variant<bool, double, std::string>> returnValue;
  int64_t snippetIdx = 0;
};
