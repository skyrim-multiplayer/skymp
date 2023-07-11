#include "ConsoleCommandsAllowedBinding.h"
#include "NapiHelper.h"

Napi::Value ConsoleCommandsAllowedBinding::Get(Napi::Env env,
                                               ScampServer& scampServer,
                                               uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  return Napi::Boolean::New(env, actor.GetConsoleCommandsAllowedFlag());
}

void ConsoleCommandsAllowedBinding::Set(Napi::Env env,
                                        ScampServer& scampServer,
                                        uint32_t formId, Napi::Value newValue)
{
  // Be careful: newValue.As<Napi::Boolean>() before static_cast<bool> is
  // strictly required
  bool consoleCommandsAllowed =
    static_cast<bool>(newValue.As<Napi::Boolean>());

  auto& partOne = scampServer.GetPartOne();
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  actor.SetConsoleCommandsAllowedFlag(consoleCommandsAllowed);
}
