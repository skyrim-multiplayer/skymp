#include "SpSnippet.h"

#include "MpActor.h"
#include "NetworkingInterface.h" // Format

SpSnippet::SpSnippet(const char* cl_, const char* func_, const char* args_)
  : cl(cl_)
  , func(func_)
  , args(args_)
{
}

void SpSnippet::Send(MpActor* actor)
{
  Networking::Format(
    [&](Networking::PacketData data, size_t len) {
      actor->SendToUser(data, len, true);
    },
    R"({"type": "spSnippet", "class": "%s", "function": "%s", "arguments": %s})",
    cl, func, args);
}