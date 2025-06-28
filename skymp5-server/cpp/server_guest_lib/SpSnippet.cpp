#include "SpSnippet.h"

#include "../../viet/include/Overloaded.h"
#include "MpActor.h"
#include "NetworkingInterface.h" // Format
#include "SpSnippetMessage.h"
#include <cmath>
#include <spdlog/spdlog.h>

SpSnippet::SpSnippet(
  const char* cl_, const char* func_,

  const std::vector<std::optional<
    std::variant<bool, double, std::string, SpSnippetObjectArgument>>>& args_,
  uint32_t selfId_)
  : cl(cl_)
  , func(func_)
  , args(args_)
  , selfId(selfId_)
{
}

Viet::Promise<VarValue> SpSnippet::Execute(MpActor* actor, SpSnippetMode mode)
{
  auto worldState = actor->GetParent();
  if (!actor->IsCreatedAsPlayer()) {
    // Return promise that never resolves in this case
    // TODO: somehow detect user instead as this breaks potential feature of
    // transferring user into an npc actor
    return Viet::Promise<VarValue>();
  }

  Viet::Promise<VarValue> promise;

  const uint32_t snippetIdx = mode == SpSnippetMode::kReturnResult
    ? actor->NextSnippetIndex(promise)
    : std::numeric_limits<uint32_t>::max();

  // Player character is always 0x14 on client, but 0xff000000+ in our server
  // See also SpSnippetFunctionGen.cpp
  auto targetSelfId =
    (selfId < 0xff000000 || selfId != actor->GetFormId()) ? selfId : 0x14;

  SpSnippetMessage message;
  message.class_ = cl;
  message.function = func;
  message.arguments = args;
  message.selfId = targetSelfId;
  message.snippetIdx = static_cast<int64_t>(snippetIdx);

  // TODO: change to SendToUser, probably was deferred only for ability to send
  // text packets
  constexpr int kChannelSpSnippet = 1;
  actor->SendToUserDeferred(message, true, kChannelSpSnippet, false);

  return promise;
}

VarValue SpSnippet::VarValueFromSpSnippetReturnValue(
  const std::optional<std::variant<bool, double, std::string>>& returnValue)
{
  if (!returnValue) {
    return VarValue::None();
  }
  return std::visit(
    Viet::Overloaded{
      [&](bool v) { return VarValue(v); },
      [&](double v) {
        auto rounded = static_cast<int32_t>(std::floor(v));
        if (std::abs(std::floor(v) - v) >
            std::numeric_limits<double>::epsilon()) {
          spdlog::error(
            "VarValueFromSpSnippetReturnValue - Floating point values are not "
            "yet supported, rounding down ({} -> {})",
            v, rounded);
        }
        return VarValue(rounded);
      },
      [&](const std::string& v) { return VarValue(v); } },
    *returnValue);
}
