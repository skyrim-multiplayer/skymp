#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct SpSnippetObjectArgument
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("formId", formId).Serialize("type", type);
  }

  uint32_t formId = 0;
  std::string type;
};

struct SpSnippetMessage : public MessageBase<SpSnippetMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::SpSnippet)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("class", class_)
      .Serialize("function", function)
      // .Serialize("arguments", arguments)
      .Serialize("selfId", selfId)
      .Serialize("snippetIdx", snippetIdx);
  }

  std::string class_;
  std::string function;
  std::vector<std::optional<
    std::variant<bool, double, std::string, SpSnippetObjectArgument>>>
    arguments;
  uint32_t selfId = 0;
  int64_t snippetIdx = 0;
};
