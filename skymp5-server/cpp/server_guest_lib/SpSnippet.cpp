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

Viet::Promise<VarValue> SpSnippet::Execute(MpActor* actorExecutor,
                                           SpSnippetMode mode)
{
  auto worldState = actorExecutor->GetParent();
  if (!actorExecutor->IsCreatedAsPlayer()) {
    // Return promise that never resolves in this case
    // TODO: somehow detect user instead as this breaks potential feature of
    // transferring user into an npc actor
    return Viet::Promise<VarValue>();
  }

  Viet::Promise<VarValue> promise;

  const uint32_t snippetIdx = mode == SpSnippetMode::kReturnResult
    ? actorExecutor->NextSnippetIndex(promise)
    : std::numeric_limits<uint32_t>::max();

  // Player character is always 0x14 on client, but 0xff000000+ in our server
  // See also SpSnippetFunctionGen.cpp
  const uint32_t targetSelfId =
    (selfId < 0xff000000 || selfId != actorExecutor->GetFormId()) ? selfId
                                                                  : 0x14;

  const uint64_t targetSelfIdLong = MakeLongFormId(worldState, targetSelfId);

  SpSnippetMessage message;
  message.class_ = cl;
  message.function = func;
  message.arguments = args;
  message.selfId = targetSelfIdLong;
  message.snippetIdx = static_cast<int64_t>(snippetIdx);

  // TODO: change to SendToUser, probably was deferred only for ability to send
  // text packets
  constexpr int kChannelSpSnippet = 1;
  actorExecutor->SendToUserDeferred(message, true, kChannelSpSnippet, false);

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

uint64_t SpSnippet::MakeLongFormId(WorldState* worldState, uint32_t formId)
{
  if (formId == 0x14 || formId >= 0xff000000) {
    return static_cast<uint64_t>(formId);
  }

  const std::shared_ptr<MpForm>& form =
    worldState->LookupFormByIdNoLoad(formId);
  auto actor = form ? form->AsActor() : nullptr;
  if (!actor) {
    return static_cast<uint64_t>(formId);
  }

  return static_cast<uint64_t>(formId) + 0x100000000;
}
