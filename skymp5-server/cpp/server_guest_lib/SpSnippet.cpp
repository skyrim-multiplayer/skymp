#include "SpSnippet.h"

#include "MpActor.h"
#include "NetworkingInterface.h" // Format
#include "SpSnippetMessage.h"

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
  message.args = args;
  message.selfId = targetSelfId;
  message.snippetIdx = static_cast<int64_t>(snippetIdx);

  // TODO: change to SendToUser, probably was deferred only for ability to send
  // text packets
  constexpr int kChannelSpSnippet = 1;
  actor->SendToUserDeferred(message, true, kChannelSpSnippet, false);

  return promise;
}
