#include "SpSnippet.h"

#include "MpActor.h"
#include "NetworkingInterface.h" // Format

SpSnippet::SpSnippet(const char* cl_, const char* func_, const char* args_,
                     uint32_t selfId_)
  : cl(cl_)
  , func(func_)
  , args(args_)
  , selfId(selfId_)
{
}

void SpSnippet::Send(MpActor* actor, std::function<void(nlohmann::json)> cb)
{
  auto snippetIdx = actor->NextSnippetIndex(cb);

  Networking::Format(
    [&](Networking::PacketData data, size_t len) {
      actor->SendToUser(data, len, true);
    },
    R"({"type": "spSnippet", "class": "%s", "function": "%s", "arguments": %s, "selfId": %u, "snippetIdx": %u})",
    cl, func, args, selfId, snippetIdx);
}